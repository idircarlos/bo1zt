#include "gui/customizer.h"
#include "logger.h"
#include "logic/config.h"
#include "gui/gui_internal.h"
#include <ui.h>

// Controller instance
static Controller *controller;
static uiWindow *parent;

// Components
static uiColorButton *scoreBgBtn;
static uiColorButton *scoreP1Btn;
static uiColorButton *scoreP2Btn;
static uiColorButton *scoreP3Btn;
static uiColorButton *scoreP4Btn;

static uiColorButton *reloadPrimaryBtn;
static uiColorButton *reloadSecondaryBtn;
static uiColorButton *lowAmmoPrimaryBtn;
static uiColorButton *lowAmmoSecondaryBtn;
static uiColorButton *noAmmoPrimaryBtn;
static uiColorButton *noAmmoSecondaryBtn;

static uiSlider *scoreboardTransparencySlider;
static uiSlider *pointsTransparencySlider;

static uiSpinbox *freqSpin;
static uiSpinbox *minSpin;
static uiSpinbox *maxSpin;

static uiButton *btnReset;
static uiButton *btnSave;

static void init();

// Handlers
static void onColorButtonChange(uiColorButton *button, void *data) {
    SimpleCheatName cheat = (SimpleCheatName)(uintptr_t)data;
    Color color = buildColor(button);
    bool success = controllerIsGameAttached(controller) ? controllerSetSimpleCheat(controller, cheat, &color) : true; // Allowing modifying checkboxes if the game is not running since they will be updated as soon as it starts.
    if (!success) {
        LOG_ERROR("Failed to set Customizer Color cheat %d to RGB(%d, %d, %d)\n", cheat, color);
        return;
    }
    uiControlEnable(uiControl(btnReset));
    uiControlEnable(uiControl(btnSave));
}

static void onSliderChange(uiSlider *slider, void *data) {
    SimpleCheatName cheat = (SimpleCheatName)(uintptr_t)data;
    int value = uiSliderValue(slider);
    bool success = controllerIsGameAttached(controller) ? controllerSetSimpleCheat(controller, cheat, &value) : true; // Allowing modifying checkboxes if the game is not running since they will be updated as soon as it starts.
    if (!success) {
        LOG_ERROR("Failed to set Customizer Slider cheat %d to %d\n", cheat, value);
        return;
    }
    uiControlEnable(uiControl(btnReset));
    uiControlEnable(uiControl(btnSave));
}

static void onSpinboxChange(uiSpinbox *spinbox, void *data) {
    SimpleCheatName cheat = (SimpleCheatName)(uintptr_t)data;
    int value = uiSpinboxValue(spinbox);
    bool success = controllerIsGameAttached(controller) ? controllerSetSimpleCheat(controller, cheat, &value) : true; // Allowing modifying checkboxes if the game is not running since they will be updated as soon as it starts.
    if (!success) {
        LOG_ERROR("Failed to set Customizer Spinbox cheat %d to %d\n", cheat, value);
        return;
    }
    uiControlEnable(uiControl(btnReset));
    uiControlEnable(uiControl(btnSave));
}

static void onResetButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    controllerResetConfig(controller, CONFIG_CUSTOMIZER);
    init();
    uiControlDisable(uiControl(btnReset));
    uiControlEnable(uiControl(btnSave));
}

static void onSaveButtonClick(uiButton *button, void *data) {
    (void)button;
    (void)data;
    controllerUpdateConfig(controller, CONFIG_CUSTOMIZER);
    uiControlDisable(uiControl(btnSave));
}

static void init() {
    CustomizerConfig config = controllerGetCustomizerConfig(controller);
    setColorButton(scoreBgBtn, config.scoreBackground);
    setColorButton(scoreP1Btn, config.scorePlayer1);
    setColorButton(scoreP2Btn, config.scorePlayer2);
    setColorButton(scoreP3Btn, config.scorePlayer3);
    setColorButton(scoreP4Btn, config.scorePlayer4);
    setColorButton(reloadPrimaryBtn, config.reloadWarnPrimary);
    setColorButton(reloadSecondaryBtn, config.reloadWarnSecondary);
    setColorButton(lowAmmoPrimaryBtn, config.lowAmmoWarnPrimary);
    setColorButton(lowAmmoSecondaryBtn, config.lowAmmoWarnSecondary);
    setColorButton(noAmmoPrimaryBtn, config.noAmmoWarnPrimary);
    setColorButton(noAmmoSecondaryBtn, config.noAmmoWarnSecondary);
    uiSliderSetValue(scoreboardTransparencySlider, config.scoreboardTransparency);
    uiSliderSetValue(pointsTransparencySlider, config.pointsTransparency);
    uiSpinboxSetValue(freqSpin, config.warningTransitionsFrequency);
    uiSpinboxSetValue(minSpin, config.warningTransitionsMin);
    uiSpinboxSetValue(maxSpin, config.warningTransitionsMax);
    uiControlDisable(uiControl(btnSave));
}

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;

    uiBox *outerBox = uiNewVerticalBox();
    uiBoxSetPadded(outerBox, 1);

    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);

    scoreBgBtn = uiNewColorButton();
    scoreP1Btn = uiNewColorButton();
    scoreP2Btn = uiNewColorButton();
    scoreP3Btn = uiNewColorButton();
    scoreP4Btn = uiNewColorButton();
    reloadPrimaryBtn = uiNewColorButton();
    reloadSecondaryBtn = uiNewColorButton();
    lowAmmoPrimaryBtn = uiNewColorButton();
    lowAmmoSecondaryBtn = uiNewColorButton();
    noAmmoPrimaryBtn = uiNewColorButton();
    noAmmoSecondaryBtn = uiNewColorButton();
    scoreboardTransparencySlider = uiNewSlider(0, 100);
    pointsTransparencySlider = uiNewSlider(0, 100);

    uiColorButtonOnChanged(scoreBgBtn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND);
    uiColorButtonOnChanged(scoreP1Btn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1);
    uiColorButtonOnChanged(scoreP2Btn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2);
    uiColorButtonOnChanged(scoreP3Btn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3);
    uiColorButtonOnChanged(scoreP4Btn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4);
    uiColorButtonOnChanged(reloadPrimaryBtn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY);
    uiColorButtonOnChanged(reloadSecondaryBtn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY);
    uiColorButtonOnChanged(lowAmmoPrimaryBtn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY);
    uiColorButtonOnChanged(lowAmmoSecondaryBtn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY);
    uiColorButtonOnChanged(noAmmoPrimaryBtn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY);
    uiColorButtonOnChanged(noAmmoSecondaryBtn, onColorButtonChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY);

    uiSliderOnChanged(scoreboardTransparencySlider, onSliderChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD);
    uiSliderOnChanged(pointsTransparencySlider, onSliderChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS);

    // --- Colores de puntuación ---
    uiGridAppend(grid, uiControl(uiNewLabel("Score Background")),           0, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);   
    uiGridAppend(grid, uiControl(scoreBgBtn),                               1, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Score Player 1")),             0, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(scoreP1Btn),                               1, 1, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Score Player 2")),             0, 2, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(scoreP2Btn),                               1, 2, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Score Player 3")),             0, 3, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(scoreP3Btn),                               1, 3, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Score Player 4")),             0, 4, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(scoreP4Btn),                               1, 4, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Reload Warn Primary")),        0, 5, 1, 1, 1, uiAlignFill, 0, uiAlignFill);    
    uiGridAppend(grid, uiControl(reloadPrimaryBtn),                         1, 5, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Reload Warn Secondary")),      0, 6, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(reloadSecondaryBtn),                       1, 6, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Low Ammo Warn Primary")),      0, 7, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(lowAmmoPrimaryBtn),                        1, 7, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Low Ammo Warn Secondary")),    0, 8, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(lowAmmoSecondaryBtn),                      1, 8, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("No Ammo Warn Primary")),       0, 9, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(noAmmoPrimaryBtn),                         1, 9, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("No Ammo Warn Secondary")),     0, 10, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(noAmmoSecondaryBtn),                       1, 10, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Scoreboard Transparency %")),  0, 11, 1, 1, 1, uiAlignFill, 0, uiAlignFill);    
    uiGridAppend(grid, uiControl(scoreboardTransparencySlider),             1, 11, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Points Transparency %")),      0, 12, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(pointsTransparencySlider),                 1, 12, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(uiNewLabel("Warning Transitions")),        0, 13, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiBox *warnBox = uiNewHorizontalBox();
    uiBoxSetPadded(warnBox, 1);

    uiGrid *warnGrid = uiNewGrid();
    uiGridSetPadded(warnGrid, 1);

    freqSpin = uiNewSpinbox(0, 10);
    minSpin = uiNewSpinbox(-200, 200);
    maxSpin = uiNewSpinbox(-200, 200);

    uiSpinboxOnChanged(freqSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY);
    uiSpinboxOnChanged(minSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN);
    uiSpinboxOnChanged(maxSpin, onSpinboxChange, (void*)SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX);

    uiGridAppend(warnGrid, uiControl(uiNewLabel("Freq (sec)")),             0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(warnGrid, uiControl(freqSpin),                             1, 0, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(warnGrid, uiControl(uiNewLabel("Min %")),                  0, 1, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(warnGrid, uiControl(minSpin),                              1, 1, 1, 1, 1, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(warnGrid, uiControl(uiNewLabel("Max %")),                  0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(warnGrid, uiControl(maxSpin),                              1, 2, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

    uiBoxAppend(warnBox, uiControl(warnGrid), 1);
    
    uiGridAppend(grid, uiControl(warnBox),                                  1, 13, 1, 1, 1, uiAlignFill, 0, uiAlignFill);

    // --- Botones inferiores ---
    uiBox *buttonBox = uiNewHorizontalBox();
    uiBoxSetPadded(buttonBox, 1);

    btnReset = uiNewButton("Reset");
    btnSave = uiNewButton("Save");

    uiButtonOnClicked(btnReset, onResetButtonClick, NULL);
    uiButtonOnClicked(btnSave, onSaveButtonClick, NULL);

    uiBoxAppend(buttonBox, uiControl(btnReset), 1);
    uiBoxAppend(buttonBox, uiControl(btnSave), 1);

    uiBoxAppend(outerBox, uiControl(grid), 1);
    uiBoxAppend(outerBox, uiControl(buttonBox), 0);

    init();

    return uiControl(outerBox);
}

static void update() {
    // Nothing for now
}


// External API for Controller
Color uiCustomizerGetCheatColor(SimpleCheatName cheat) {
    switch (cheat) {
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_BACKGROUND:
            return buildColor(scoreBgBtn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P1:
            return buildColor(scoreP1Btn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P2:
            return buildColor(scoreP2Btn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P3:
            return buildColor(scoreP3Btn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_SCORE_P4:
            return buildColor(scoreP4Btn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_PRIMARY:
            return buildColor(reloadPrimaryBtn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_RELOAD_SECONDARY:
            return buildColor(reloadSecondaryBtn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_PRIMARY:
            return buildColor(lowAmmoPrimaryBtn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_LOW_AMMO_SECONDARY:
            return buildColor(lowAmmoSecondaryBtn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_PRIMARY:
            return buildColor(noAmmoPrimaryBtn);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_NO_AMMO_SECONDARY:
            return buildColor(noAmmoSecondaryBtn);
        default:
            LOG_ERROR("Unknown cheat %d\n", cheat);
            return buildColor(scoreBgBtn); // Just return random color
    }
}

int uiCustomizerGetCheatInt(SimpleCheatName cheat) {
    switch (cheat) {
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_SCOREBOARD:
            return uiSliderValue(scoreboardTransparencySlider);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_TRANSPARENCY_POINTS:
            return uiSliderValue(pointsTransparencySlider);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_FREQUENCY:
            return uiSpinboxValue(freqSpin);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MIN:
            return uiSpinboxValue(minSpin);
        case SIMPLE_CHEAT_NAME_CUSTOMIZER_WARN_MAX:
            return uiSpinboxValue(maxSpin);
        default:
            LOG_ERROR("Unknown cheat %d\n", cheat);
            return 0;
    }
}

bool uiCustomizerIsSavable() {
    return uiControlEnabled(uiControl(btnSave));
}

void uiCustomizerReset() {
    init();
}

UIControlGroup *uiCustomizerBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
