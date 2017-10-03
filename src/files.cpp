// Copyright (c) 1989-94 James E. Wilson, Robert A. Koeneke
//
// Umoria is free software released under a GPL v2 license and comes with
// ABSOLUTELY NO WARRANTY. See https://www.gnu.org/licenses/gpl-2.0.html
// for further details.

// Misc code to access files used by Moria

#include "headers.h"

// This must be included after fcntl.h, which has a prototype for `open' on some
// systems.  Otherwise, the `open' prototype conflicts with the `topen' declaration.
#include "externs.h"

//  initializeScoreFile
//  Open the score file while we still have the setuid privileges.  Later
//  when the score is being written out, you must be sure to flock the file
//  so we don't have multiple people trying to write to it at the same time.
//  Craig Norborg (doc)    Mon Aug 10 16:41:59 EST 1987
bool initializeScoreFile() {
    highscore_fp = fopen(MORIA_SCORES, (char *) "rb+");

    return highscore_fp != nullptr;
}

// Attempt to open and print the file containing the intro splash screen text -RAK-
void displaySplashScreen() {
    vtype_t in_line = {'\0'};

    FILE *file1 = fopen(MORIA_SPLASH, "r");
    if (file1 != nullptr) {
        clearScreen();
        for (int i = 0; fgets(in_line, 80, file1) != CNIL; i++) {
            putString(in_line, Coord_t{i, 0});
        }
        waitForContinueKey(23);

        (void) fclose(file1);
    }
}

// Open and display a text help file
// File perusal, primitive, but portable -CJS-
void displayTextHelpFile(const char *filename) {
    constexpr uint8_t max_line_length = 80;

    char line_buffer[max_line_length];

    FILE *file = fopen(filename, "r");

    if (file == nullptr) {
        (void) sprintf(line_buffer, "Can not find help file \"%s\".\n", filename);
        putStringClearToEOL(line_buffer, Coord_t{0, 0});
        return;
    }

    terminalSaveScreen();

    char input;
    while (feof(file) == 0) {
        clearScreen();

        for (int i = 0; i < 23; i++) {
            if (fgets(line_buffer, max_line_length - 1, file) != CNIL) {
                putString(line_buffer, Coord_t{i, 0});
            }
        }

        putStringClearToEOL("[Press any key to continue.]", Coord_t{23, 23});
        input = getKeyInput();
        if (input == ESCAPE) {
            break;
        }
    }

    (void) fclose(file);

    terminalRestoreScreen();
}

// Prints a list of random objects to a file. -RAK-
// Note that the objects produced is a sampling of objects
// which be expected to appear on that level.
void outputRandomLevelObjectsToFile() {
    obj_desc_t input = {0};

    putStringClearToEOL("Produce objects on what level?: ", Coord_t{0, 0});
    if (!getStringInput(input, 0, 32, 10)) {
        return;
    }

    int level;
    if (!stringToNumber(input, level)) {
        return;
    }

    putStringClearToEOL("Produce how many objects?: ", Coord_t{0, 0});
    if (!getStringInput(input, 0, 27, 10)) {
        return;
    }

    int count;
    if (!stringToNumber(input, count)) {
        return;
    }

    if (count < 1 || level < 0 || level > 1200) {
        putStringClearToEOL("Parameters no good.", Coord_t{0, 0});
        return;
    }

    if (count > 10000) {
        count = 10000;
    }

    bool small_objects = getInputConfirmation("Small objects only?");

    putStringClearToEOL("File name: ", Coord_t{0, 0});

    vtype_t filename = {0};

    if (!getStringInput(filename, 0, 11, 64)) {
        return;
    }
    if (strlen(filename) == 0) {
        return;
    }

    FILE *file_ptr = fopen(filename, "w");
    if (file_ptr == nullptr) {
        putStringClearToEOL("File could not be opened.", Coord_t{0, 0});
        return;
    }

    (void) sprintf(input, "%d", count);
    putStringClearToEOL(strcat(input, " random objects being produced..."), Coord_t{0, 0});

    putQIO();

    (void) fprintf(file_ptr, "*** Random Object Sampling:\n");
    (void) fprintf(file_ptr, "*** %d objects\n", count);
    (void) fprintf(file_ptr, "*** For Level %d\n", level);
    (void) fprintf(file_ptr, "\n");
    (void) fprintf(file_ptr, "\n");

    int treasure_id = popt();

    for (int i = 0; i < count; i++) {
        int object_id = itemGetRandomObjectId(level, small_objects);
        inventoryItemCopyTo(sorted_objects[object_id], treasure_list[treasure_id]);

        magicTreasureMagicalAbility(treasure_id, level);

        Inventory_t &item = treasure_list[treasure_id];
        itemIdentifyAsStoreBought(item);

        if ((item.flags & TR_CURSED) != 0u) {
            itemAppendToInscription(item, ID_DAMD);
        }

        itemDescription(input, item, true);
        (void) fprintf(file_ptr, "%d %s\n", item.depth_first_found, input);
    }

    pusht((uint8_t) treasure_id);

    (void) fclose(file_ptr);

    putStringClearToEOL("Completed.", Coord_t{0, 0});
}

// Write character sheet to the file
static void writeCharacterSheetToFile(FILE *file1) {
    putStringClearToEOL("Writing character sheet...", Coord_t{0, 0});
    putQIO();

    const char *colon = ":";
    const char *blank = " ";

    vtype_t statDescription = {'\0'};

    (void) fprintf(file1, "%c\n\n", CTRL_KEY('L'));

    (void) fprintf(file1, " Name%9s %-23s", colon, py.misc.name);
    (void) fprintf(file1, " Age%11s %6d", colon, (int) py.misc.age);
    statsAsString(py.stats.used[A_STR], statDescription);
    (void) fprintf(file1, "   STR : %s\n", statDescription);
    (void) fprintf(file1, " Race%9s %-23s", colon, character_races[py.misc.race_id].name);
    (void) fprintf(file1, " Height%8s %6d", colon, (int) py.misc.height);
    statsAsString(py.stats.used[A_INT], statDescription);
    (void) fprintf(file1, "   INT : %s\n", statDescription);
    (void) fprintf(file1, " Sex%10s %-23s", colon, (playerGetGenderLabel()));
    (void) fprintf(file1, " Weight%8s %6d", colon, (int) py.misc.weight);
    statsAsString(py.stats.used[A_WIS], statDescription);
    (void) fprintf(file1, "   WIS : %s\n", statDescription);
    (void) fprintf(file1, " Class%8s %-23s", colon, classes[py.misc.class_id].title);
    (void) fprintf(file1, " Social Class : %6d", py.misc.social_class);
    statsAsString(py.stats.used[A_DEX], statDescription);
    (void) fprintf(file1, "   DEX : %s\n", statDescription);
    (void) fprintf(file1, " Title%8s %-23s", colon, playerTitle());
    (void) fprintf(file1, "%22s", blank);
    statsAsString(py.stats.used[A_CON], statDescription);
    (void) fprintf(file1, "   CON : %s\n", statDescription);
    (void) fprintf(file1, "%34s", blank);
    (void) fprintf(file1, "%26s", blank);
    statsAsString(py.stats.used[A_CHR], statDescription);
    (void) fprintf(file1, "   CHR : %s\n\n", statDescription);

    (void) fprintf(file1, " + To Hit    : %6d", py.misc.display_to_hit);
    (void) fprintf(file1, "%7sLevel      : %7d", blank, (int) py.misc.level);
    (void) fprintf(file1, "    Max Hit Points : %6d\n", py.misc.max_hp);
    (void) fprintf(file1, " + To Damage : %6d", py.misc.display_to_damage);
    (void) fprintf(file1, "%7sExperience : %7d", blank, py.misc.exp);
    (void) fprintf(file1, "    Cur Hit Points : %6d\n", py.misc.current_hp);
    (void) fprintf(file1, " + To AC     : %6d", py.misc.display_to_ac);
    (void) fprintf(file1, "%7sMax Exp    : %7d", blank, py.misc.max_exp);
    (void) fprintf(file1, "    Max Mana%8s %6d\n", colon, py.misc.mana);
    (void) fprintf(file1, "   Total AC  : %6d", py.misc.display_ac);
    if (py.misc.level >= PLAYER_MAX_LEVEL) {
        (void) fprintf(file1, "%7sExp to Adv : *******", blank);
    } else {
        (void) fprintf(file1, "%7sExp to Adv : %7d", blank, (int32_t) (player_base_exp_levels[py.misc.level - 1] * py.misc.experience_factor / 100));
    }
    (void) fprintf(file1, "    Cur Mana%8s %6d\n", colon, py.misc.current_mana);
    (void) fprintf(file1, "%28sGold%8s %7d\n\n", blank, colon, py.misc.au);

    int xbth = py.misc.bth + py.misc.plusses_to_hit * BTH_PER_PLUS_TO_HIT_ADJUST + (class_level_adj[py.misc.class_id][CLASS_BTH] * py.misc.level);
    int xbthb = py.misc.bth_with_bows + py.misc.plusses_to_hit * BTH_PER_PLUS_TO_HIT_ADJUST + (class_level_adj[py.misc.class_id][CLASS_BTHB] * py.misc.level);

    // this results in a range from 0 to 29
    int xfos = 40 - py.misc.fos;
    if (xfos < 0) {
        xfos = 0;
    }
    int xsrh = py.misc.chance_in_search;

    // this results in a range from 0 to 9
    int xstl = py.misc.stealth_factor + 1;
    int xdis = py.misc.disarm + 2 * playerDisarmAdjustment() + playerStatAdjustmentWisdomIntelligence(A_INT) + (class_level_adj[py.misc.class_id][CLASS_DISARM] * py.misc.level / 3);
    int xsave = py.misc.saving_throw + playerStatAdjustmentWisdomIntelligence(A_WIS) + (class_level_adj[py.misc.class_id][CLASS_SAVE] * py.misc.level / 3);
    int xdev = py.misc.saving_throw + playerStatAdjustmentWisdomIntelligence(A_INT) + (class_level_adj[py.misc.class_id][CLASS_DEVICE] * py.misc.level / 3);

    vtype_t xinfra = {'\0'};
    (void) sprintf(xinfra, "%d feet", py.flags.see_infra * 10);

    (void) fprintf(file1, "(Miscellaneous Abilities)\n\n");
    (void) fprintf(file1, " Fighting    : %-10s", statRating(12, xbth));
    (void) fprintf(file1, "   Stealth     : %-10s", statRating(1, xstl));
    (void) fprintf(file1, "   Perception  : %s\n", statRating(3, xfos));
    (void) fprintf(file1, " Bows/Throw  : %-10s", statRating(12, xbthb));
    (void) fprintf(file1, "   Disarming   : %-10s", statRating(8, xdis));
    (void) fprintf(file1, "   Searching   : %s\n", statRating(6, xsrh));
    (void) fprintf(file1, " Saving Throw: %-10s", statRating(6, xsave));
    (void) fprintf(file1, "   Magic Device: %-10s", statRating(6, xdev));
    (void) fprintf(file1, "   Infra-Vision: %s\n\n", xinfra);

    // Write out the character's history
    (void) fprintf(file1, "Character Background\n");
    for (auto &entry : py.misc.history) {
        (void) fprintf(file1, " %s\n", entry);
    }
}

static const char *equipmentPlacementDescription(int itemID) {
    switch (itemID) {
        case EQUIPMENT_WIELD:
            return "You are wielding";
        case EQUIPMENT_HEAD:
            return "Worn on head";
        case EQUIPMENT_NECK:
            return "Worn around neck";
        case EQUIPMENT_BODY:
            return "Worn on body";
        case EQUIPMENT_ARM:
            return "Worn on shield arm";
        case EQUIPMENT_HANDS:
            return "Worn on hands";
        case EQUIPMENT_RIGHT:
            return "Right ring finger";
        case EQUIPMENT_LEFT:
            return "Left  ring finger";
        case EQUIPMENT_FEET:
            return "Worn on feet";
        case EQUIPMENT_OUTER:
            return "Worn about body";
        case EQUIPMENT_LIGHT:
            return "Light source is";
        case EQUIPMENT_AUX:
            return "Secondary weapon";
        default:
            return "*Unknown value*";
    }
}

// Write out the equipment list.
static void writeEquipmentListToFile(FILE *file1) {
    (void) fprintf(file1, "\n  [Character's Equipment List]\n\n");

    if (equipment_count == 0) {
        (void) fprintf(file1, "  Character has no equipment in use.\n");
        return;
    }

    obj_desc_t description = {'\0'};
    int itemSlotID = 0;

    for (int i = EQUIPMENT_WIELD; i < PLAYER_INVENTORY_SIZE; i++) {
        if (inventory[i].category_id == TV_NOTHING) {
            continue;
        }

        itemDescription(description, inventory[i], true);
        (void) fprintf(file1, "  %c) %-19s: %s\n", itemSlotID + 'a', equipmentPlacementDescription(i), description);

        itemSlotID++;
    }

    (void) fprintf(file1, "%c\n\n", CTRL_KEY('L'));
}

// Write out the character's inventory.
static void writeInventoryToFile(FILE *file1) {
    (void) fprintf(file1, "  [General Inventory List]\n\n");

    if (inventory_count == 0) {
        (void) fprintf(file1, "  Character has no objects in inventory.\n");
        return;
    }

    obj_desc_t description = {'\0'};

    for (int i = 0; i < inventory_count; i++) {
        itemDescription(description, inventory[i], true);
        (void) fprintf(file1, "%c) %s\n", i + 'a', description);
    }

    (void) fprintf(file1, "%c", CTRL_KEY('L'));
}

// Print the character to a file or device -RAK-
bool outputPlayerCharacterToFile(char *filename) {
    vtype_t msg = {'\0'};

    int fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0 && errno == EEXIST) {
        (void) sprintf(msg, "Replace existing file %s?", filename);
        if (getInputConfirmation(msg)) {
            fd = open(filename, O_WRONLY, 0644);
        }
    }

    FILE *file;
    if (fd >= 0) {
        // on some non-unix machines, fdopen() is not reliable,
        // hence must call close() and then fopen().
        (void) close(fd);
        file = fopen(filename, "w");
    } else {
        file = nullptr;
    }

    if (file == nullptr) {
        if (fd >= 0) {
            (void) close(fd);
        }
        (void) sprintf(msg, "Can't open file %s:", filename);
        printMessage(msg);
        return false;
    }

    writeCharacterSheetToFile(file);
    writeEquipmentListToFile(file);
    writeInventoryToFile(file);

    (void) fclose(file);

    putStringClearToEOL("Completed.", Coord_t{0, 0});

    return true;
}
