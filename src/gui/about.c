#include "gui/about.h"
#include "metadata.h"
#include "resource_ids.h"


// Controller instance
static Controller *controller;

// Parent Window instance
static uiWindow *parent;

static uiControl *build(Controller *controllerInstance, uiWindow *parentInstance) {
    controller = controllerInstance;
    parent = parentInstance;

    // Main group
    uiGroup *aboutGroup = uiNewGroup("About");
    uiBox *mainBox = uiNewHorizontalBox();
    uiBoxSetPadded(mainBox, 1);

    // Left side: Icon
    uiImageView *iconView = uiNewImageView(90, 90);
    uiImageViewSetFromResource(iconView, IDR_PNG_ICON);
    uiBoxAppend(mainBox, uiControl(iconView), 0);

    // Right side: Info
    uiBox *infoBox = uiNewVerticalBox();
    uiBoxSetPadded(infoBox, 1);

    uiLabel *titleLabel = uiNewLabel("Black Ops 1 Zombies Trainer v" BO1ZT_VERSION);
    uiLink *discordLink = uiNewLink("Join our discord community", BO1ZT_DISCORD_URL);
    uiLink *githubLink = uiNewLink("See source code on Github", BO1ZT_GITHUB_URL);
    uiLabel *authorLabel = uiNewLabel("By " BO1ZT_BY);

    uiBoxAppend(infoBox, uiControl(titleLabel), 0);
    uiBoxAppend(infoBox, uiControl(discordLink), 0);
    uiBoxAppend(infoBox, uiControl(githubLink), 0);
    uiBoxAppend(infoBox, uiControl(authorLabel), 0);

    uiBoxAppend(mainBox, uiControl(infoBox), 1);

    uiGroupSetChild(aboutGroup, uiControl(mainBox));
    uiGroupSetMargined(aboutGroup, 1);
    return uiControl(aboutGroup);
}

static void update() {
    // Nothing
}

UIControlGroup *uiAboutBuildControlGroup() {
    UIControlGroup *cg = guiControlGroupCreate(build, update);
    return cg;
}
