# Camo Manager

A **camo** is a named set of *.iwi* textures. A **bundle** is a list of weapon to camo assignments. The game only ever sees one installed bundle.

## Camos

Four slots per camo, each one a different job inside the material:

- **Spec** carries the reflections. Files are prefixed with `~~-`.
- **Color** carries the base color. Files are prefixed with `~-`.
- **Env** carries glow and reflection, and is sometimes used as a second color layer. Files are prefixed with `~`.
- **Normal** carries the heights and depths of the surface, and doubles as reflection when it is transparent. Files are not prefixed.

The prefixes only describe how the game names its own files. What you add can be named anything, the slot you drop it into is what decides its role.

`Add Extra File` creates a numbered variant of one of the four slots, for weapons that need more than one texture of the same type.

Files are copied into the trainer's own storage, so the originals can be moved or deleted afterwards.

## Bundles

`Install` copies the bundle's textures into the game folder. The game has to be restarted after each installation. Installing a bundle replaces whatever was installed before. Deleting the installed bundle uninstalls it first.

`Uninstall` removes those files again, also requiring a restart.

Deleting the installed bundle uninstalls it first. Also, if you edit a camo that the installed bundle uses, the installed files are rewritten right away.

## Assignments

An assignment needs three selections: a bundle, a weapon and a camo. Selecting them changes nothing on its own. The button below the weapon list is the only thing that writes to the bundle, and its label states what it will do, for example `Assign Gold to AK47`. Pressing it again on the same weapon and camo removes the assignment.

## Viewer

Shows the selected weapon with the selected camo whether or not that pair is assigned, so any combination can be previewed first.

Drag with the left mouse button to orbit, wheel to zoom.

The **Color**, **Normal**, **Spec** and **Env** checkboxes hide individual textures, which is the fastest way to tell what a single file contributes.
