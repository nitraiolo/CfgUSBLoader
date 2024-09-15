# Configurable SD/USB Loader v70r78 MOD patched

![enter image description here](http://cfgusbloader.ntd.homelinux.org/images/USB.png)

by oggzee, usptactical, gannon, Dr. Clipper, R2-D2199, FIX94, airline38,
   Howard, NiTRo THe DeMoN

This version of the loader allows you to customize numerous options
to better suit your preferences.

  Based on Wanikoko SD/USB Loader 1.5, Kwiirk Yal & cios 222,
  Hermes uLoader 1.6 & cios 222/223+mload + many others (Sorg, nIxx,
  fishears, usptactical, 56Killer, WiiShizzza, hungyip84, Narolez, ...)

## Official Site
http://cfgusbloader.ntd.homelinux.org/

## Features

 - SDHC and USB HDD device support
 - GUI (Grid / Coverflow) and Console mode (switchable at runtime)
 - Background Music (.mp3 or .mod)
 - Themes (switchable at runtime)
 - Themes Browser
 - Widescreen (auto-detect)
 - Transparency (covers and console)
 - Cover images download
 - Cover styles: 2d, 3d, disc and full
 - Automatic resize of covers
 - Renaming game titles (using titles.txt, custom-titles.txt and/or wiitdb.zip)
 - Per game configuration of Video mode, Language, Ocarina cheating
 - Light up DVD slot when install finishes, optional eject
 - Childproof and parental guidance
 - USB HDD with multiple partitions supported
   (WBFS for games and FAT for config, covers and other resources)
 - UStealth MBR supported
 - SDHC with multiple partitions supported
   (WBFS for games and FAT for resources...)
 - Custom IOS selection for better compatibility with USB drives and other USB devices.
 - cIOS supported:
   davebaol d2x (recommended)
   waninkoko 247, 248, 249, 250 251 & 252
   Hermes 222, 223 & 224 (mload)
   kwiirk 222 & 223 (yal)
 - Banner sounds
 - Saving game play time to Wii's play log
 - Multiple WBFS partitions support
 - Loading games from a FAT or NTFS partition
   (cIOS 222/223 rev4+ or cIOS 249 rev18+ required)
 - Supported game disc file formats: .wbfs and .iso
 - Configurable
 

For latest news, questions and suggestions head over here:
http://gbatemp.net/index.php?showtopic=147638


## Installation:

Copy the contents of "inSDroot" to the root of your SD card.
This will add the USB Loader to your homebew channel.
You can also install a a channel forwarder to start the loader directly
from the system menu.
(Note: alternatively, a USB FAT partition can be used
to store configuration and covers)

## Useful links:

Step-by-step installation instructions for Wii Homebrew and Cfg:
http://gwht.wikidot.com

The forwarder can be downloaded from here:
http://gbatemp.net/index.php?showtopic=147638&st=5745&p=2420653&#entry2420653
And installed with the WAD Manager.

Game cover images can be downloaded from here:
http://wiitdb.com

Updated titles.txt can found here:
http://wiitdb.com/titles.txt


## Default File locations:

      config file:     sd:/usb-loader/config.txt
      settings file:   sd:/usb-loader/settings.cfg
      background image:sd:/usb-loader/background.png
      covers:
        2d:            sd:/usb-loader/covers/2d/*.png
        3d:            sd:/usb-loader/covers/3d/*.png
        disc:          sd:/usb-loader/covers/disc/*.png
        full (and HQ): sd:/usb-loader/covers/full/*.png
        cache:         sd:/usb-loader/covers/cache/*.ccc
      titles file:     sd:/usb-loader/titles.txt
      themes:          sd:/usb-loader/themes/THEME_NAME/theme.txt
      theme preview:   sd:/usp-loader/themes/*.jpg
    
      ocarina cheat codes are searched in:
                       sd:/usb-loader/codes/*.gct
                       sd:/data/gecko/codes/*.gct
                       sd:/codes/*.gct
      ocarina TXT cheat codes use the locations:
        Downloaded to: sd:/usb-loader/codes/*.txt
        Saved .gct to: sd:/usb-loader/codes/*.gct
    
      wiitdb:          sd:/usb-loader/wiitdb.zip
      play history:    sd:/usb-loader/playstats.txt
    
      .wip files:      sd:/usb-loader/GAMEID.wip
      .bca files:      sd:/usb-loader/GAMEID.bca
      .wdm files:      sd:/usb-loader/GAMEID.wdm
    
      nand Immage      usb:/nand
    
      Games on FAT/NTFS
        :              usb:/wbfs/GAMEID.wbfs
      or:              usb:/wbfs/TITLE [GAMEID].wbfs
      or:              usb:/wbfs/GAMEID_TITLE/GAMEID.wbfs
      or:              usb:/wbfs/TITLE [GAMEID]/GAMEID.wbfs
      (or sd:/ instead of usb:/)

These are default locations, background and covers are configurable.
If config file is not found in sd:/usb-loader, then it is searched
in: sd:/apps/USBLoader/config.txt (or whichever directory is used to
start from homebrew channel) and if found, the base path is set to
that location.

Besides the global config file sd:/usb-loader/config.txt, also
the config.txt in the apps directory is used, which by default is:
sd:/apps/USBLoader/config.txt but depends from where the loader
has been started from the homebrew channel.

Another way to specify a config file is by using the wiiload
utility from a PC, which allows additional arguments:
C:\> wiiload usbloader.dol config21.txt
which will load sd:/usb-loader/config21.txt
And you can also pass config options directly:
C:\> wiiload usbloader.dol video=ntsc

So you can have different configurations depending on where it
is started from. This can be useful so you can configure the
loader started from forwarder to be simple/childproff, while
if started from homebrew channel to include the installation and
removal options.
To achieve this, copy sd:/apps/USBLoader/ to sd:/apps/USBLoader_admin
and then set simple=1 in sd:/apps/USBLoader/config.txt and edit (or delete)
sd:/apps/USBLoader_admin/meta.xml to show a different name.


## USB FAT for config and covers:

SD card is first searched for config.txt, if not found then USB HDD
with a FAT partition is mounted as usb:/ and searched, for config.txt.
in that case, all the paths mentioned above will be searched
on usb:/ instead of sd:/
Advantage of using USB FAT for config and covers is faster loading of
covers. (Usually USB HDD devices are faster than SD cards)
If you want to load the application from SD card but use USB for config
and covers, then copy everything in inSDroot to your USB FAT partition,
but only the inSDroot/apps directory to SD card and
delete sd:/apps/USBLoader/config.txt and sd:/usb-loader/config.txt.

Note: To create 2 partitions on SD or USB thumb drives you have to use
special tools like gparted: http://gparted.sourceforge.net/
Create 2 primary partitions, format both as FAT32 and then inside the loader
re-format the SECOND partition to WBFS. Suggested partitions size if 200MB
for config & covers and the rest for WBFS.

Note: If you have problems accessing the fat partition make sure it is
marked 'active' in the windows partition editor (or if you use gparted,
mark the partition with the 'boot' flag)

Limitations:
- USB FAT is mounted only if config.txt is not found on SD.


Themes:
-------

Another way to customize the loader is by using Themes.
A theme is defined by creating a config file in the location:
sd:/usb-loader/themes/THEME_NAME/theme.txt
And the background images:
sd:/usb-loader/themes/THEME_NAME/background.png
sd:/usb-loader/themes/THEME_NAME/background_wide.png

The options that can be used in theme.txt are described below,
basically only options that define the looks are accepted, everything else
is ignored in theme.txt. Background image (background.png or specified with
background option) for a theme will be searched first in the theme
directory (sd:/usb-loader/themes/THEME_NAME/) and if not found
it will be searched in the base directory (sd:/usb-loader/)

Other themable gui resources:
  favorite.png
  pointer.png
  hourglass.png
  font_uni.png (new type 512 letters unicode)
  font.png (old type 128 letters ascii)
  font_clock.png
  window.png, page.png
  button.png, radio.png, checkbox.png
Same search method as for the background image applies to these files.
There are some example files in: usb-loader/resources, but note that the program
does not search in that location, they need to be copied to the base or theme
directory to be used.

Background overlay support:
additional images can be supplied that will be overlaid over the background:
 - bg_overlay.png or bg_overlay_w.png for console background
 - bg_gui_over.png or bg_gui_over_w.png for gui background
If *_w.png variant is not found then the normal is used.
If a theme is used then the overlay files are searched only in theme directories,
not in base directories.

Auto-scaling of background image:
Since version 41 background width can be larger than 640 and will be either
 - scaled if widescreen
 - cropped if not widescreen
down to 640x480. Note: height must still be 480.

GUI buttons, icons and windows theme images:
button images: button.png, radio.png, checkbox.png
window images: window.png, page.png
States: button images can contain a single button image
or multiple images for different states:
- normal, flash, hover, press, press hover, inactive
Any number of buttons can be present, if some are missing then
the normal state + coloring and effects are used to generate the missing state.
A button width:height must be 2:1, so that the number of states can be
calculated: num = height * 2 / width
Scaling: a button is divided to 3 areas horizontally: 1/4 1/2 1/4 the first
and last are scale proportionally while the middle is streched as much as required.
Round buttons only use the first and last parts.
Similarily a window image is divided also to 3 areas vertically and same rules apply.
The image width and height must be divisible by 4.


GUI Mode:
---------

By default console mode is started.
To switch to GUI mode, press B in main screen.
While in GUI mode, the following buttons are used by default (they can be remapped):

    button A : start selected game
    button B : return to console mode
    button LEFT/RIGHT (Grid): previous/next page
    button LEFT/RIGHT (Coverflow) : rotate to next cover - hold down for fast rotate.
    button UP/Down (Grid) : switch number of rows (1-3)
    button UP (Coverflow) : flip center cover to its back/front
    button DOWN : change gui styles (grid -> flow -> flow-z -> coverflow 3d -> etc)
    button 1, +, - : options, install, remove
    button 2: favorites game list
    button HOME : exit

The background image in GUI mode can be changed with the file:

    sd:/usb-loader/themes/Theme_Name/bg_gui.png  or
    sd:/usb-loader/bg_gui.png


## Downloading covers

Covers can be downloaded for all games at once or on a per-game basis.
To download the covers for all games go to Global Options screen, move
to "<Download All Covers>" and press button RIGHT or LEFT.
There is a difference between LEFT and RIGHT buttons:
- button RIGHT will download all MISSING covers.
- button LEFT will download ALL covers, even the ones that are already present.
To download the covers for the current selected game, go to Game Options screen,
move to "Cover Image:" option and press RIGHT or LEFT. The same difference between
RIGHT and LEFT buttons apply here as with download all covers.
The way how the covers are downloaded can be further customized with config options
download_* and url_*. See the documentation on these options below in the Config file section.

## Parental Control and Unlocking Admin Functionality

If the loader is configured with restricted functionality it can be unlocked
with a password - a "secret" wiimote button combination. Restricted functionality
means any of the options: "simple" or "disable_*" or "hide_game" are set, which is
usually used for "Parental Control".

When admin mode is unlocked it will allow you to access all the previously 
locked functionality. In addition: when unlocked, any games that are hidden
with the hide_game option will be displayed.

To access the unlock screen, hold the 1 button down for 5 seconds and the screen
will appear.  After you see the text "Enter Code:", press the wiimote buttons
in the correct order.  If you were successful, the word SUCCESS will appear
on the screen.  Otherwise the word LOCKED! will appear.  The unlock screen has a 30
second timeout limit so if an incorrect (or no) password is entered, it will
automatically lock.  To set the lock back on with the original settings intact, hold
the 1 button for 5 seconds and the lock will automatically turn on.  When the loader
is started, the lock will always be enabled.

This functionality is controlled by the option: admin_unlock = [1], 0.
By default it is enabled. To disable the ability to unlock admin mode set
the option to: admin_unlock = 0

The default admin unlock password is: BUDAH12
To change it, use the config option: unlock_password = [BUDAH12]
The password length is limited to 10 characters. Do NOT use quotes around the
password - just type what you want it to be.  E.g.  unlock_password = 12UDAB
The following are the button to letter mappings for the password:

      D-Pad Up:     U
      D-Pad Down:   D
      D-Pad Right:  R
      D-Pad Left:   L
      B button:     B
      A Button:     A
      Minus button: M
      Plus button:  P
      Home button:  H
      1 button:     1
      2 button:     2

To hide certain games, use the the Game Options screen and toggle the "Hide Game"
setting.  This allows you to set which games are hidden when the admin mode is LOCKED.
In order to see this option, admin_unlock must be enabled (which it is by default) AND
the admin mode must be in an unlocked state!
- **NOTE**: this functionality completely replaces the hide_game option in config.txt,
    but they CAN be used together. Any games currently listed in hide_game will be
    shown when unlocked, but will ALWAYS be marked as hidden by default in Game Options
       and cannot be changed to unhidden unless they are removed from config.txt
- **NOTE 2**: An easy way to convert all your games in hide_game to this new functionality
    is to start the loader with your hide_game still in config.txt and then go into
    Game Options in any game (you may have to unlock admin lock first) and change
    something and save it. All your hide_game entries will automatically be saved.  
    Then you can remove the hide_game entry completely from config.txt.


## Various Controllers Button Mapping:
There are 17 possible buttons, listed here with the controller mappings.

    -----------------------------------------------------------
    BUTTON   WIIMOTE  CLASSIC  GC       GUITAR       NUNCHUK
    -----------------------------------------------------------
    UP       UP       UP       UP       STRUM UP
    RIGHT    RIGHT    RIGHT    RIGHT    NONE
    DOWN     DOWN     DOWN     DOWN     STRUM DOWN
    LEFT     LEFT     LEFT     LEFT     NONE
    -        -        -                 -
    +        +        +                 +
    A        A        A        A        GREEN        
    B        B        B        B        RED          
    HOME     HOME     HOME     START    
    1        1                          
    2        2                          
    X                 X        X        BLUE
    Y                 Y        Y        YELLOW
    Z                 ZL/ZR    Z        ORANGE       Z
    C                                                C
    L                 L        L
    R                 R        R

The actions of these buttons can be changed via the button_* options.

## Title Rename

To rename a game title, edit the file in sd:/usb-loader/titles.txt
or sd:/usb-loader/custom-titles.txt
Format is:

    GAMEID = Game Title

Example:

    RSPP = Wii Sports



## Config file:

    # lines starting with # are comments
    # default values are in [ ]
    #
    # There are 2 categories of options: Global and Theme
    #   Theme options (theme.txt) only affect the looks, everything else is ignored
    #   Global options (config.txt) will accept all options, including theme options.
    #
    #
    # Theme options:
    # ==============
    #
    # background = [background.png]
    # wbackground = [background_wide.png]
    #   file to use for the background image.
    #   file can be an absolute path (like sd:/somedir/myimage.png)
    #   if the specified background is not an absolute path it is searched
    #   in theme directory (sd:/usb-loader/themes/THEME_NAME/)
    #   and in default directory (sd:/usb-loader)
    #
    # background_base = [background.png]
    # wbackground_base = [background_wide.png]
    #   base background image over which the bg_overlay* files are overlaid.
    #   actually this option is the same as normal background option
    #   the only difference is that it is understood by versions that
    #   support overlaid background, which makes it possible to create
    #   forward and backward compatible themes
    #
    # background_gui = [bg_gui.png]
    # wbackground_gui = [bg_gui_wide.png]
    #  gui mode background file.
    #
    # layout = original2 original small medium large large2 [large3],
    #          ultimate1 ultimate2 ultimate3 kosaic
    #   original:   6 lines (wanikoko 1.0-1.1)
    #   original2: 12 lines (wanikoko 1.2-1.5)
    #   small:      9 lines (same background as original)
    #   medium:    17 lines
    #   large:     21 lines (nixx)
    #   large2:    21 lines (usptactical)
    #   large3:    21 lines (oggzee) 
    #   ultimate1: 12 lines (WiiShizza)
    #   ultimate2: 12 lines (jservs7 / hungyip84)
    #   ultimate3: 12 lines (WiiShizza - cover on right)
    #
    #
    # covers = [1], 0
    #   (enable/disable covers)
    #
    # console_coords = x,y,width,height
    # wconsole_coords = x,y,w,h (widescreen)
    #
    # console_color = foreground,background
    #   (color values: 0-15)
    #   Dark:  0=black, 1=red, 2=green, 3=yellow, 4=blue, 5=purple, 6=cyan, 7=bright grey
    #   Bright: 8=grey, 9=red, 10=green, 11=yellow, 12=blue, 13=purple, 14=cyan, 15=white
    #   see: http://www.silverpoint.com/leo/color/c-16.gif (0 at top to 15 at bottom)
    #
    # covers_coords = x,y
    # wcovers_coords = x,y (widescreen)
    #
    # hide_header = [0],1 
    # hide_hddinfo = 0,[1]
    # hide_footer = [0],1 
    #   These options control the look of main menu.
    #
    # simple = [0], 1
    #   Using the simple option in a theme will only affect the hide_footer option
    #   it will not change the disable_* options. Use the simple option in the
    #   global config.txt to change the disable_* options as well.
    #
    # colors = [dark], bright, mono
    #   will select a prefedines set of colors for a dark or bright background
    #   or normal 2 color text if mono is specified
    #
    # color_header, color_selected_fg, color_selected_bg,
    # color_inactive, color_footer, color_help
    #   Will set individual text colors. Values are 0-15
    #
    # cover_style = [standard], 3d, disc
    #   Support for 3d covers and disc covers
    #   This option selects which cover_url and cover_path is used
    #
    # console_transparent = [0], 1
    #   Enable transparent console.
    #
    # covers_size = width, height
    #   default: covers_size = 160, 225
    #   If the loaded image is if different size, it will be to the size specified.
    #
    # wcovers_size = width, height
    #   default: wcovers_size = 130, 225
    #   used for widescreen setting. If not set it will use the covers_size
    #   and properly scaling it to widescreen size.
    #
    # cursor = ">>"
    #   Changes cursor shape, at max 2 characters are used. Can be empty.
    #   If you want spaces, so that the menu is not shifted use quotes
    #   like this: cursor = "  "
    #
    # menu_plus  = "[+] "
    #   Changes the [+] mark in the main, options and start menu.
    #   At max 4 characters are used.
    #
    # gui_text_color = [black], white, RRGGBBAA
    #   Set the text color in GUI mode
    #   Note: to use a color other than a black or white, it has to be
    #   represented as a HEX value with the following components: RRGGBBAA
    #   RR=red, GG=green, BB=blue, AA=alpha
    #   Example: red = FF0000FF, blue = 00FF00FF, yellow = 00FFFFFF
    #
    # gui_text_outline = [FF], AA, RRGGBBAA, black, white
    # gui_text_shadow = [00], AA, RRGGBBAA, black, white
    #   Outline and shadow color for text in GUI mode
    #   If only alpha is specified, it will set the outline / shadow
    #   color to black or white depending if text color is bright or dark.
    #
    # gui_text2_color = [white], black, RRGGBBAA
    # gui_text2_outline = [FF], AA, RRGGBBAA, black, white
    # gui_text2_shadow = [00], AA, RRGGBBAA, black, white
    #   Text colors for coverflow modes, where background is darker
    #
    # gui_title_top = [0], 1
    #   If 1, gui text appears above covers.  If 0, below.
    #
    # coverflow_reflection = color_top, color_bottom
    #   color is hex rgba, 0,0 will disable reflections
    #   default: coverflow_reflection = 666666FF, AAAAAA33 
    #
    # gui_cover_area = x, y, w, h
    #   default: gui_cover_area = 20, 24, 600, 408
    #   minimum accepted w, h: 480, 320
    #   enabling debug will draw the area rectangle
    #
    # gui_title_area = x, y, w, h
    #   default: 0,0,0,0 meaning, position depends on gui_title_top
    #   min w, h: 320, 10 note: h is not yet used
    #
    # gui_clock_pos = x, y
    #   default: -1,-1 meaning, title position is used
    #   clock position, if set clock is displayed all the time
    #
    # gui_page_pos = x, y
    #   default: -1,-1 meaning, title position is used
    #   specifies the page indicator position
    #
    # preview_coords = x,y,width,height
    # wpreview_coords = x,y,width,height   (widescreen)
    #   Settings for the Theme Preview image.
    #   If X and/or Y is set to -1, the Cover X and/or Y position is used.
    #   Either W or H (or both) should be set to zero to maintain the correct 
    #   aspect ratios.  If both are set to zero, the Cover width is used and the 
    #   height is scaled accordingly.
    #
    # home = [reboot], exit, hbc, screenshot, priiloader, wii_menu, 
    #        <magic word>, <channel ID>
    #   What to do when an exit button (typically home) is pressed in the menus
    #   Also changes the operation of button_H in the gui and game list.
    #   <magic word> is a 4 letter magic word for Priiloader
    #   <Channel ID> is a 4 letter channel ID in upper case to launch
    #   If home=screenshot then home has to be held for 1 second to make a screenshot
    #   otherwise it will exit to HBC
    #
    # buttons=[options_1], options_B, original
    #   (change button controls layout in the gui and game list.)
    #
    #   The button layout "options_1" is:
    #       button B - GUI mode
    #       button 1 - options menu
    #
    #   The button layout "options_B" is:
    #       button B - options menu
    #       button 1 - GUI mode
    #
    #   The button layout "original" is obsolete, but is:
    #       button 1 - options menu
    #       button B - nothing
    #
    # button_B = [gui], <other actions>, <magic word>, <channel ID>
    # button_- = [main_menu], <other actions>, <magic word>, <channel ID>
    # button_+ = [install], <other actions>, <magic word>, <channel ID>
    # button_H = [reboot], <other actions>, <magic word>, <channel ID>
    # button_1 = [options], <other actions>, <magic word>, <channel ID>
    # button_2 = [favorites], <other actions>, <magic word>, <channel ID>
    # button_X = A, B, 1, [2], H, -, +, <action>, <magic word>, <channel ID>
    # button_Y = A, B, [1], 2, H, -, +, <action>, <magic word>, <channel ID>
    # button_Z = A, [B], 1, 2, H, -, +, <action>, <magic word>, <channel ID>
    # button_C = [A], B, 1, 2, H, -, +, <action>, <magic word>, <channel ID>
    # button_L = A, B, 1, 2, H, [-], +, <action>, <magic word>, <channel ID>
    # button_R = A, B, 1, 2, H, -, [+], <action>, <magic word>, <channel ID>
    #   These buttons can be mapped to any of the following actions in the game list and gui:
    #     nothing    # does nothing
    #     options    # access game options
    #     gui        # switch to/from GUI
    #     reboot     # reboot to system menu
    #     exit       # exit to launching app
    #     hbc        # exit to HBC
    #     screenshot # take a screenshot
    #     install    # install a game
    #     remove     # remove a game
    #     main_menu  # access main menu 
    #     global_ops # access global options menu
    #     favorites  # toggle favorites view
    #     boot_game  # boot a game from the drive
    #     boot_disc  # boot a game from disc
    #     theme      # switch to next theme
    #     profile    # switch to next profile
    #     unlock     # access the unlock password dialog immediately
    #     sort       # switch to next sort
    #     filter     # access filter menu
    #     priiloader # access Priiloader via magic word Daco
    #     wii_menu   # get Priiloader to launch Wii Menu via magic word Pune
    #     <magic word> # is a 4 letter magic word for Priiloader
    #     <Channel ID> # is a 4 letter channel ID in upper case to launch
    #   X, Y, Z, C, L & R can also be optionally targetted to emulate one of the buttons
    #   on the Wiimote (A, B, 1, 2, -, +, Home).  This emulation will function everywhere.
    #
    # button_cancel = [B], <comma separated list of buttons>
    #   Set which button(s) will act as the back button in menus
    #   Valid button values in the list are: 
    #     B, 1, 2, -, M, Minus, +, P, Plus, H, Home, X, Y, Z, C, L, R
    #
    # button_exit = [Home], <comma separated list of buttons>
    #   Set which button(s) will perform the 'home' action in menus
    #   Valid button values in the list are: 
    #     B, 1, 2, -, M, Minus, +, P, Plus, H, Home, X, Y, Z, C, L, R
    #
    # button_other = [1], <comma separated list of buttons>
    #   Set which button(s) will perform the other or alternate action in menus
    #   This covers switching between options and global options, choosing to
    #   download BCA during install, choosing to ignore meta.xml during upgrade etc.
    #   Valid button values in the list are: 
    #     B, 1, 2, -, M, Minus, +, P, Plus, H, Home, X, Y, Z, C, L, R
    #   
    # button_save = [2], <comma separated list of buttons>
    #   Set which button(s) will perform the save action in menus
    #   Valid button values in the list are: 
    #     B, 1, 2, -, M, Minus, +, P, Plus, H, Home, X, Y, Z, C, L, R
    # 
    # gui_window_color_base = RRGGBBAA default: FFFFFF80
    # gui_window_color_popup = RRGGBBAA default: FFFFFFB0
    #
    # gui_text_color_menu = COLOR / OUTLINE / SHADOW
    # gui_text_color_info = COLOR / OUTLINE / SHADOW
    # gui_text_color_title = COLOR / OUTLINE / SHADOW
    # gui_text_color_button = COLOR / OUTLINE / SHADOW
    # gui_text_color_radio = COLOR / OUTLINE / SHADOW
    # gui_text_color_checkbox = COLOR / OUTLINE / SHADOW
    #   default:
    #   gui_text_color_menu = white / A0 / 44444444
    #   gui_text_color_info = white / A0
    #   each component can be "black", "white" or RRGGBBAA
    #   OUTLINE and SHADOW are optional
    #   Setting gui_text_color_menu will set all the colors except info
    #   Setting gui_text_color_button will also set radio and checkbox
    #
    # gui_button_NAME = X, Y, W, H, TextColor, image.png, Type, HoverZoom
    #   NAME can be: main settings quit style view sort filter favorites jump
    #   TextColor is same format as gui_text_color_menu
    #   (Seting TextColor to 0 will disable text on the button in case icons are used)
    #   Type: button or icon
    #   HoverZoom: 0-50 in %
    #   Paramteres after coordinates are optional. Default values:
    #   gui_button_NAME = X, Y, W, H, white/A0/44444444, button.png, button, 10
    #
    # gui_bar = 0, [1], top, bottom
    #   Will disable / enable gui bar or enable only top or bottom.
    #
    # Global options:
    # ===============
    #
    # gui = 0, 1, 2, 3, [4], start
    #   gui = 0 : GUI disabled
    #   gui = 1 : GUI enabled, GUI Menu disabled, start in console mode
    #   gui = 2 : GUI enabled, GUI Menu disabled, start in GUI mode
    #   gui = 3 : GUI Menu enabled, start in console mode
    #   gui = 4 : GUI Menu enabled, start in GUI mode
    #   gui = start : same as gui = 4
    #
    # gui_transition = [scroll], fade
    #   Set GUI transition effect between console and gui mode
    #
    # gui_style = [grid], flow, flow-z, coverflow3d, coverflow2d, frontrow, vertical, carousel
    #   Set the default GUI style
    #
    # gui_rows = [2] (Valid values: 1-4)
    #   Set the default number of rows in GUI mode
    #
    # gui_lock = [0], 1
    #   Disable changing gui style with button up/down
    #
    # gui_antialias = [4] {0-4}
    #   Tune coverflow mode antialias level
    #
    # gui_pointer_scroll = 0, [1]
    #   disable/enable pointer scrolling of the game list
    #
    # theme = Theme_Name
    #   Load a specified theme from sd:/usb-loader/themes/Theme_Name/theme.txt
    #   Note: a theme resets all theme related options, so if you want to override
    #   any theme option, do that in the lines after the 'theme =' option.
    #
    # covers_path = [sd:/usb-loader/covers]
    #   Changing covers_path will change all covers_path_* like this: 
    # covers_path_2d = covers_path/2d
    # covers_path_3d = covers_path/3d
    # covers_path_disc = covers_path/disc
    # covers_path_full = covers_path/full
    #   If you need fine controll on the 2d/3d/disc/full paths use the
    #   covers_path_* options after covers_path.
    #
    # URL options:
    #   URL can contain any of the following tags which are then replaced
    #   with proper values: {REGION}, {WIDTH}, {HEIGHT}, {ID3}, {ID4}, {ID6}, {ID}, {CC}, {PUB}
    #   {ID} will find automatically the correct ID by trying ID6, ID4 and ID3
    #   Note: {WIDTH} and {HEIGHT} don't work well if download_all_styles is enabled
    #   {PUB} os the last two digits of the ID and can be used to get covers from other
    #   regions.  E.g., replace {ID6} with {ID3}P{PUB} to look for PAL covers.
    #
    #   Multiple URLS can be specified for cover_url_* options
    #   They can be in the same line separated with space, example:
    #   cover_url = http://site1.com/{ID}.png http://site2.org/{ID}.png
    #   Or in multiple lines using =+ to add instead of =, example:
    #   cover_url = http://site1.com/{ID}.png
    #   cover_url =+ http://site2.org/{ID}.png
    #   cover_url =+ http://site3.net/{ID}.png
    #
    # Normal 4:3 mode aspect ratio url options:
    # cover_url = URL      (url for standard 2d flat covers)
    # cover_url_3d = URL   (url for 3d covers)
    # cover_url_disc = URL (url for disc covers)
    # cover_url_full = URL (url for full covers)
    # cover_url_hq = URL   (url for HQ full covers)
    #
    #   defaults:
    #
    #   cover_url = http://wiitdb.com/wiitdb/artwork/cover/{CC}/{ID6}.png
    #
    #   cover_url_3d = http://wiitdb.com/wiitdb/artwork/cover3D/{CC}/{ID6}.png
    #
    #   cover_url_disc =
    #   cover_url_disc =+ http://wiitdb.com/wiitdb/artwork/disc/{CC}/{ID6}.png
    #   cover_url_disc =+ http://wiitdb.com/wiitdb/artwork/disccustom/{CC}/{ID6}.png
    #
    #   cover_url_full = http://wiitdb.com/wiitdb/artwork/coverfull/{CC}/{ID6}.png
    #
    #   cover_url_hq = http://wiitdb.com/wiitdb/artwork/coverfullHQ/{CC}/{ID6}.png
    #
    # download_id_len = 4, [6]
    #   Specifies the downloaded cover ID length for the saved file name 
    #
    # download_all_styles = [1], 0
    #   Download all cover styles (flat,3d,disc), or just the current style
    #
    # download_cc_pal = [AUTO], EN, FR, DE, AU, ...
    #   The code is used as a replacement for {CC} tag for PAL regions.
    #   If AUTO is specified, then the code is set depending on the console
    #   country - if Australia: AU, Brasil: PT else, console language is used.
    #   If image is not found with the specified cc code, download is
    #   retried with EN code.
    #
    # titles_url = http://wiitdb.com/titles.txt?LANG={DBL}
    #   Url for updating titles.txt
    #
    # device = [ask], usb, sdhc
    #   Specifies which device to use when starting the loader
    #
    # partition = [auto], ask, WBFS1, ...WBFS4, FAT1, ...FAT9, NTFS1, ...NTFS9
    #   Specifies which partition to use when starting the loader
    #   Note: the -222 version defaults to FAT1 instead.
    #
    # simple = [0], 1
    #   (enable/disable simple/childsafe mode)
    #   setting simple=1 will change all disable_xxx options and
    #   hide_footer to 1 and confirm_start to 0.
    #   setting simple=0 will do the opposite.
    #   any of the disable_xxx, hide_xxx and confirm_start options can be set individually.
    #
    # confirm_start = [1], 0
    #   Specifies if confirmation is required when starting a game
    #
    # disable_remove = [0],1
    # disable_install = [0],1
    # disable_options = [0],1
    # disable_format = [0],1
    #   fine permissions control 
    #
    # admin_unlock = [1], 0.
    #   If enabled it will allow unlock admin mode, so you will be able to access
    #   all the settings screens that simple or disable_* options disabled.
    #   
    # unlock_password = [BUDAH12]
    #   Set the password for admin unlock
    #
    # install_partitions = [only_game], all, 1:1, iso
    #   controls which partitions from DVD are installed to HDD
    #   - 1:1 disable scrubbing, except for the last 256kb which are stil scrubbed
    #     because of a wbfs limitation: the wbfs block size not aligned to wii disc size
    #   - iso: On NTFS it creates an exact dump to an iso file
    #     On WBFS/FAT it will behave same as 1:1
    #
    # fat_split_size = [4], 2, 0
    #   Selects if the split is at 4: 4gb-32kb or 2: 2gb-32kb
    #   or 0: no splits - ntfs only
    #
    # fat_install_dir = [1], 0, 2, 3
    #   Select install filename layout on fat:
    #   fat_install_dir = 0 /wbfs/GAMEID.wbfs
    #   fat_install_dir = 1 /wbfs/GAMEID_TITLE/GAMEID.wbfs
    #   fat_install_dir = 2 /wbfs/TITLE [GAMEID]/GAMEID.wbfs
    #   fat_install_dir = 3 /wbfs/TITLE [GAMEID].wbfs
    # fs_install_layout is an alias for fat_install_dir
    #
    # ntfs_write = [0], 1, norecover
    #   0 will disable and 1 will enable ntfs write support
    #   norecover will enable write but prevent mounting an uncleanly unmounted fs
    #     in case recovery on the PC is preferred.
    #
    # music = [1], 0, filename, PATH
    #   Play background music (only one option has to be specified)
    #   Examples:
    #   music = 1
    #     (which is default) will randomly play all .mp3 or .mod
    #     files (whichever are found first) in sd:/usb-loader dir
    #   music = sd:/usb-loader/my_music.mp3
    #     Plays my_music.mp3. The file name can be specified
    #     relative to sd:/usb-loader or an absolute pathname.
    #   music = sd:/music
    #     Will randomly play all .mp3 or .mod files in that folder.
    #   music = 0
    #     Will disable music
    #
    # widescreen = [auto], 0, 1
    #   * If widescreen is enabled (or autodetected) then the following options are used:
    #      - wbackground
    #      - wbackground_base
    #      - wbackground_gui
    #      - wconsole_coords
    #      - wcovers_coords
    #      - wcovers_size
    #   * when in widescreen mode, cover images will be searched first by
    #     the name GAMEID_wide.png and if not found then by GAMEID.png and
    #     will be resized properly.
    #   * some layouts will specify widescreen cooridinates automatically
    #     like: large3 and ultimate3, so there is no need to specify them manually,
    #     if one of these layouts are used.
    #
    # hide_game = [0], GAMEID1, GAMEID2, ...
    #   Hide games from list (can be used for parental control)
    #   Multiple games can be specified in one line separated by comma ","
    #   or each game in a separate hide_game = GAMEID line.
    #   setting hide_game = 0 will reset the hide list.
    #   GAMEID is a 4 letter game ID.
    #   Example: hide_game = RZZP, RDCP
    #
    # pref_game = [0], GAMEID1, GAMEID2, ...
    #   Preffered games, to be shown first in the list.
    #   Syntax is same as with hide_game.
    #   Example: pref_game = RHAP, RSSP
    #
    # confirm_ocarina = [0], 1
    #   Specifies if confirmation is required when ocarina is enabled
    #   and codes have been found
    #
    # cursor_jump = [0] or N
    #   Sets how much moves left/right (if 0 do a end page / next page jump)
    #
    # start_favorites = [0], 1
    #   Start with the favorites game list
    #
    # clock_style = [24], 12, 12am, 0
    #   Set clock style, 0 will disable clock
    #
    # sort_ignore = [A,An,The], ...
    #   Which starting words to ignore when sorting titles
    #   Separate words via commas, do not incude the square brackets
    #   Set sort_ignore = 0 for old sort method - to not ignore anything
    #
    # debug = [0], 1, 8
    #   Display some diagnostic messages when started
    #   Benchmark mode with debug = 8 and start or install a game
    #
    # debug_gecko = [0],1,2,3
    #   write debug info to usb gecko (1=debug,2=console,3=both)
    #
    # profile_names = [default], name2, name3,...
    #   Profiles - aka multiple favorite groups
    #   (max profiles:10, max profile name length: 16)
    #   Profiles can be added, removed and reordered with this option.
    #   But if you want to rename it, you will need to change the profile
    #   name also in settings.cfg otherwise it will be considered as a new
    #   name and the old one will be forgotten next time you save settings.
    #   Profiles can be switched in the global options screen. Changing a
    #   game favorite setting is done as usual in the game options screen.
    #
    # profile = name
    #   will specify the default profile to use
    #   saving global settings will also save which profile is used
    #
    # profile_start_favorites = {[0], 1} ...
    #   specifies when each profile is switched to if favorites should be enabled or disabled.
    #   profile_start_favorites 0 1 1 1 1 1 1 1 1 1
    # 
    # db_url = [http://wiitdb.com/wiitdb.zip?LANG={DBL}]
    #   URL to download database from.
    #
    # db_language = [AUTO], EN, JA, German, etc
    #   Language to use for the {DBL} tag. If invalid or not able to be
    #   displayed by the loader this will default to English.
    #   Both country codes (EN) and languages (English) are valid.
    #
    # db_show_info = [1], 0
    #   Show info loaded from the database.
    #
    # db_ignore_titles = [0], 1
    #   Set this to ignore titles from the database when naming games.
    #
    # write_playstats = [1], 0
    #   Write to the play history file.
    #
    # sort = [title-asc], etc
    #   Change the default sorting method. Default is Title Ascending.
    #
    #   Valid sort options:
    #     "title"         => Title
    #     "players"       => Number of Players
    #     "online_players"=> Number of Online Players
    #     "publisher"     => Publisher
    #     "developer"     => Developer
    #     "release"       => Release Date
    #     "play_count"    => Play Count
    #     "install"       => Install Date
    #                        (This will only work with FAT or NTFS drives)
    #     "play_date      => Last Play Date
    #
    #   To use ascending add "-asc" to the option.
    #     ie: sort = players-asc
    #   
    #   To use descending add "-desc" to the option.
    #     ie: sort = players-desc
    #
    # translation = [AUTO], EN, custom, etc.
    #   Current auto values: JA, EN, DE, FR, ES, IT, NL, ZH, ZH, KO
    #   Any filename is supported as long as a corresponding translation 
    #   file exists. Translation files currently exist for DE, DK, ES, FI, FR,
    #   GR, IT, JA, KO, NL, NO, PT_BR, PT_PT, TR, ZH_CN, ZH_CN-clamis, ZH_TW
    #
    # load_unifont = [0], 1
    #   Specifies if unifont.dat should be loaded or not. unifont.dat contains
    #   all the unicode characters, required for ASIAN language support so that
    #   translation or wiitdb info shows up correctly.
    #   load_unifont automaticly switches to 1 if translation starts with JA, KO or ZH
    #   or if db_language is JA, KO, ZH
    #   Note: the LATIN unicode set is already embedded into the loader,
    #   so to display German, French, Spanish, etc... unifont.dat is not needed
    #
    # wiird = [0], 1, 2
    #   1 = enable debugger
    #   2 = enable debugger and pause start
    #
    # select = [previous], start, middle, end, most, least, random
    #   Selects which game is picked by default on startup.
    #   The new default is the previous game played (to get old
    #   operation, set select=start).
    #   'start', 'middle' and 'end' refer to position in the list.
    #   'most' and 'least' refer to number of plays (in Cfg).
    #   'random' selects a different game each time.
    #
    # adult_themes = [0], 1
    #   determines if adult themes can be downloaded
    #
    # theme_previews = [1], 0
    #   determines if the theme preview images should be downloaded and displayed
    #
    # return_to_channel = [0], auto, JODI, FDCL, ...
    #   Games will return to the selected channel ID
    #   e.g., JODI for HBC or a forwarder channel like FDCL or DCFG
    #     to reload Cfg.
    #   0 is the default Wii Menu operation
    #   auto: will try to detect the channel id from where the loader was started.
    #   (although some forwarders are not auto detected properly,
    #    but the official one by FIX94 is)
    #
    # disable_nsmb_patch = [0],1
    # disable_pop_patch = [0],1
    # disable_dvd_patch = 0,[1]
    #   Optionally disable patches performed by the loader.
    #
    #   nsmb = New Super Mario Bros.
    #   pop = Prince of Persia: The Forgotten Sands
    #   dvd = disc check patch for Hermes.
    #
    #   PoP requires that you disable the dvd patch
    #   Set an option to 1 to disable (i.e., not patch)
    #
    # gamercard_url = URL
    # gamercard_key = key
    #   These options work like the cover_url options
    #   and support the =+ operator to add multiple sites/keys.
    #   Keys and URLs are matched up in respective order.
    #
    #   If you set a key in the list to 0, or leave out trailing keys, the
    #   respective sites from the URLs will not be tried.
    #
    #   The tags {KEY} and {ID6} can be used in the URLs.
    #
    #   Defaults are for WiinnerTag, NCard and DUTag in that order, but with blank keys:
    #   gamercard_url =  http://www.wiinnertag.com/wiinnertag_scripts/update_sign.php?key={KEY}&game_id={ID6}
    #   gamercard_url =+ http://www.messageboardchampion.com/ncard/API/?cmd=tdbupdate&key={KEY}&game={ID6}
    #   gamercard_url =+ http://tag.darkumbra.net/{KEY}.update={ID6}
    #   gamercard_key = 0 0 0
    #
    # intro = 0, 1, [2], 3
    #   intro=0 : black - only allowed when direct game launching
    #   intro=1 : black bg with program name (small)
    #   intro=2 : color image [default]
    #   intro=3 : grey image
    #   This option only works if set in meta.xml <arguments>
    #
    # nand_emu_path = [usb:/nand]
    #   sets the path whare a nand image is located. It must be a fat drive.
    #
    # save_filter = [0], 1
    #   enables games filters persistance on a per profile basis.
    #
    # custom_private_server = [wiimmfi.de], STRING
    #   custom WFC server. Max len 14.
    #
    # nintendont_config_mode = [file],arg
    #   file = parameters are passed to nintendont rewriting nintendont config file
    #   arg = parameters are passed to nintendont via command line argument
    #
    # update_nintendont = [0],1
    #   enables nintendont update with other plugins.
    #
    # Game Compatibility Options:
    # ===========================
    # 
    # These options can be set in the global config.txt
    # The options can be set also per-game from inside the loader
    # options screen and will be saved to settings.cfg
    #
    # video = [auto], game, system, pal50, pal60, ntsc
    #   (auto is same as game)
    #
    # language = [console], japanese, english, german, french, spanish,
    #            italian, dutch, s.chinese, t.chinese, korean
    #
    # video_patch = [0], 1, all, sneek, sneek+all
    #   'video_patch = 1': This will patch NTSC -> PAL modes if console is PAL
    #   and PAL -> NTSC modes if console is NTSC
    #   This will not change interlaced / progressive mode
    #   'video_patch = all' will force all modes to the selected video mode.
    #   This will also force progressive / interlaced mode, depending on what
    #   is configured in wii settings. This can be used for example to force
    #   progressive mode if the game will otherwise use interlaced mode (MPT)
    #
    # vidtv = [0], 1
    #   Required by some games (Japanese,...)
    #
    # country_patch = [0], 1
    #   Country Patch for better compatibility with some games
    #
    # fix_002 = [0], 1
    #   This is the Anti 002 fix.  It is not needed for new cIOSes (IOS249 rev14+)
    #
    # ios = 247, 248, [249], 222-mload, 223-mload, 224-mload, 222-yal, 223-yal, 250
    #   Select Custom IOS
    #   Note: 222-yal is for kwiirk's cIOS
    #   Note: 222-mload is for Hermes's cIOS and is the default for the -222 version
    #   Note: a few seconds of delay when starting a game with custom ios is expected.
    #
    # block_ios_reload = 0, 1, [auto]
    #   Required by some games, works with d2x cios and in a limited way for hermes cios
    #   0: disabled, 1: enabled, auto: enable if cios is d2x and ver >= 5
    #
    # alt_dol = [0], 1, sd, disc
    #   Alternative .dol loading option (from NeoGamma by WiiPower)
    #   If set to 1 or sd it will load the .dol from
    #     the loader base dir + GAMEID.dol (4 letter ID)
    #     [ sd:/usb-loader/GAMEID.dol ]
    #   If set to disc it will prompt with a list of .dols on the wbfs iso image
    #
    # ocarina = [0], 1
    #   enable/disable ocarina - cheating engine
    #
    # hooktype = nohooks, [vbi], wiipad, gcpad, gxdraw, gxflush, ossleep, axframe
    #   specify ocarina hook type
    #
    # write_playlog = [0], 1, 2, 3
    #   Write gameplay stats to the Wii message board log.
    #   This option won't work when the Wii Menu is skipped before
    #   starting Cfg (e.g., using Priiloader or BootMii autoboot).
    #   0 = off, do not write to the Wii Message Board log.
    #   1 = on, set title based on the value of the language option unless
    #       the title in that language is blank where it then uses the
    #       English title or the Japanese title (whichever is valid).
    #   2 = on, set title to the Japanese title.
    #   3 = on, set title to the English title.
    #   In the per-game menu, these options show up as "Off", "On",
    #   "Japanese Title" and "English Title" respectively.
    #
    # clear_patches = [0],1,all
    #   When on (1) return_to_channel and the dvd check are disabled
    #   When 'all', then all .dol patches are disabled
    #
    # mem_card_emu = [0], 1, 2
    #   Emulation GC memory card:
    #     0 = disabled
    # 	  1 = enabled with separate memory card per game
    # 	  2 = enabled with shared memory card
    #
    # mem_card_size = [0], 1, 2, 3, 4, 5
    #   Emulated GC memory card size:
    #     0 = 59 blocks (512KB)
    #     1 = 123 blocks (1MB)
    #     2 = 251 blocks (2MB)
    #     3 = 507 blocks (4MB)
    #     4 = 1019 blocks (8MB)
    #     5 = 2043 blocks (16MB)
    #
    # private_server = [0],1,2,3
    #   Nintendo Wi-Fi Connection (WFC) patching:
    #     0 = Off
    #     1 = NoSSL only
    #     2 = wiimmfi.de
    #     3 = Custom string from "custom_private_server"
    #	
    # fix_480p = [0], 1
    #   enable/disable 480p Nintendo Revolution SDK bug fix patch
    #
    # deflicker = [0], 1, 2, 3, 4, 5
    #   override deflicker filter settings:
    #     0 = Defaults as hardcoded in the game
    #     1 = Turns off deflickering setting the vfilter to Off
    #     2 = Turns off deflickering setting the vfilter and patchig GXSetCopyFilter function
    #     3 = Turns on deflickering setting vfilter to Low
    #     4 = Turns on deflickering setting vfilter to Medium
    #     5 = Turns on deflickering setting vfilter to High
    #
    # dithering = [0], 1
    #   enable/disable dithering removal patch
    #
    # fix_fb = [0], 1
    #   enable/disable framebuffer width patch
    #
    # Profile Options:
    # ================
    # 
    # The options can be set per-profile from inside the loader
    # and will be saved to settings.cfg
    #
    # profile_filter = [all], online, unplayed, genre(*), features(*), controller(*), gamecube, wii, channel, duplicate, game_type(*), search(*)
    #   (*) = requires extra options
    #
    # profile_filter_genre = action, adventure, fighting, music, platformer, puzzle, racing, role-playing, shooter, simulation, sports, strategy, arcade, baseball, basketball, bike_racing, billiards, board_game, bowling, boxing, business_simulation, cards, chess, coaching, compilation, construction_simulation, cooking, cricket, dance, darts, drawing, educational, exercise, first-person_shooter, fishing, fitness, flight_simulation, football, futuristic_racing, golf, health, hidden_object, hockey, hunting, karaoke, kart_racing, life_simulation, management_simulation, martial_arts, motorcycle_racing, off-road_racing, party, petanque, pinball, poker, rail_shooter, rhythm, rugby, sim_racing, skateboarding, ski, snowboarding, soccer, stealth_action, surfing, survival_horror, table_tennis, tennis, third-person_shooter, train_simulation, trivia, truck_racing, virtual_pet, volleyball, watercraft_racing, wrestling
    #
    # profile_filter_feature = online, download, score, nintendods
    #
    # profile_filter_controller = wiimote, nunchuk, motionplus, gamecube, nintendods, classiccontroller, wheel, zapper, balanceboard, microphone, guitar, drums, camera, dancepad, infinitybase, keyboard, portalofpower, skateboard, totalbodytracking, turntable, udraw, wiispeak, vitalitysensor
    #
    # profile_filter_gametype = wii, gamecube, all_channels, wiiware, vc_nes, vc_snes, vc_n64, vc_sms, vc_md, vc_pce, vc_neogeo, vc_arcade, vc_c64, wii_channels, fwd_emu
    #
    # profile_search_field = title, synopsis, developer, publisher, id6, region, rating, players, online_players, play_count, synopsis_len, covers_available
    #
    # profile_search_string = STRING
    #   Max len 100 chars.
    #

## Config file sample:

    # USBLoader config
    # lines starting with # are comments
    # see README-CFG.txt for help on options
    # theme
    theme = GreyMatter
    # gui
    gui = start
    gui_transition = fade
    gui_style = flow
    gui_rows = 2
    # device
    device = usb
    ntfs_write = 1
    # language related options:
    # translation = XX
    # db_language = XX
    # this is required for asian languages:
    # load_unifont = 1
    # NAND Emulation (wiiware/VC)
    nand_emu_path = usb:/nand
    # Return to loader
    return_to_channel = UCXF
    # Multiple profile filter config
    profile_names = sample1, sample2
    save_filter = 1
    # GC memory card emulation
    mem_card_emu = 1
    mem_card_size = 2
    # Nintendont 
    nintendont_config_mode = arg
    update_nintendont = 1

## Sample titles.txt file:

    RFNP = Wii Fit
    RHAP = Wii Play
    RSPP = Wii Sports
    RMGP = Super Mario Galaxy

## Changelog:

See ChangeLog.txt
