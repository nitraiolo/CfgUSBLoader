
Configurable SD/USB Loader v70
==============================

by oggzee, usptactical, gannon & Dr. Clipper

This version of the loader allows you to customize numerous options
to better suit your preferences.

  Based on Wanikoko SD/USB Loader 1.5, Kwiirk Yal & cios 222,
  Hermes uLoader 1.6 & cios 222/223+mload + many others (Sorg, nIxx,
  fishears, usptactical, 56Killer, WiiShizzza, hungyip84, Narolez, ...)


Features:
---------

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
 - SDHC with multiple partitions supported
   (WBFS for games and FAT for resources...)
 - Custom IOS selection for better compatibility with USB drives and other USB devices.
 - cIOS supported:
   davebaol d2x (recommended)
   waninkoko 247, 248, 249 & 250
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


Installation:
-------------

Copy the contents of "inSDroot" to the root of your SD card.
This will add the USB Loader to your homebew channel.
You can also install a a channel forwarder to start the loader directly
from the system menu.
(Note: alternatively, a USB FAT partition can be used
to store configuration and covers)

Useful links:
-------------

Step-by-step installation instructions for Wii Homebrew and Cfg:
http://gwht.wikidot.com

The forwarder can be downloaded from here:
http://gbatemp.net/index.php?showtopic=147638&st=5745&p=2420653&#entry2420653
And installed with the WAD Manager.

Game cover images can be downloaded from here:
http://wiitdb.com

Updated titles.txt can found here:
http://wiitdb.com/titles.txt


Default File locations:
---------------

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


USB FAT for config and covers:
------------------------------

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


Downloading covers
------------------

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

Parental Control and Unlocking Admin Functionality
--------------------------------------------------

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
- NOTE: this functionality completely replaces the hide_game option in config.txt,
    but they CAN be used together. Any games currently listed in hide_game will be
    shown when unlocked, but will ALWAYS be marked as hidden by default in Game Options
       and cannot be changed to unhidden unless they are removed from config.txt
- NOTE 2: An easy way to convert all your games in hide_game to this new functionality
    is to start the loader with your hide_game still in config.txt and then go into
    Game Options in any game (you may have to unlock admin lock first) and change
    something and save it. All your hide_game entries will automatically be saved.  
    Then you can remove the hide_game entry completely from config.txt.


Various Controllers Button Mapping:
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

Title Rename
------------

To rename a game title, edit the file in sd:/usb-loader/titles.txt
or sd:/usb-loader/custom-titles.txt
Format is:
GAMEID = Game Title
Example:
RSPP = Wii Sports



Config file:
------------


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

Config file sample:
-------------------

# USBLoader config
background = background.png
layout = large3
covers = 1
hide_footer = 1
# device = usb


Sample titles.txt file:
-----------------------

RFNP = Wii Fit
RHAP = Wii Play
RSPP = Wii Sports
RMGP = Super Mario Galaxy



Changelog:
----------

07-07-2011 cfg v70 (release)
 * Version
 * Full package changes:
 - New default theme: Glass (by The-Magician)

05-07-2011 cfg v70b6 (beta)
 * Updated to rodries ehcmodule for hermes cios (222,223,224)

03-07-2011 cfg v70b5 (beta)
 * Fixed occasional crashes in GUI game select
   especially when using WBFS partition
 * Minor cleanups

02-07-2011 cfg v70b4 (beta)
 * Print game requested IOS in game info screen
   (after the GAMEID and size) (tnx R2-D2199)
 * Minor cleanups

02-07-2011 cfg v70b3 (beta)
 * Support for hdd with 4k sector size on fat/ntfs/wbfs filesystems
   Thanks to: dimok (libs) davebaol (cios) and dexter (testing)

28-06-2011 cfg v70b2 (beta)
 * Fixed GUI game options when clear patches is set to all
 * Improved exception stack dump output to show also the debug log and version
 * Improved gui theme switch to reflect text and button color changes immediately

26-06-2011 cfg v70b (beta)
 * Updated devkitPPC to 23
 * Updated libogc to 1.8.7
 * When clear patches is set to all really disable all game patches,
   previously some could still be enabled (anti 002 and some others)
 * Fixed GUI Menu display of game options hook type when ocarina is enabled

25-06-2011 cfg v70a (alpha)
 * updated libraries for improved support of hdd with 4k sectors: (tnx dimok)
   libfat r4700, libntfs r10 (ntfs-3g 2011.4.12), libext2fs r15 (v1.0.2)

15-06-2011 cfg v69d (bugfix)
 * Fixed wbfs on hdd with 4k sectors (broken by v69a6)
   (And disabled fat & ntfs on hdd with 4k sec. since it doesn't work)
 * Updated "about" credits and translators

12-06-2011 cfg v69c (bugfix)
 * Fixed crash when return_to_channel is set to an invalid value
   (non-existing channel id)

09-06-2011 cfg v69 (release)
 * Updated about window with credits & translators list
 * Full package changes:
 - New default theme: Blue 2011 (by The-Magician)
 - Added tetris.mp3

07-06-2011 cfg v69b3 (beta)
 * changed option: block_ios_reload = 0, 1, [auto]
   'auto' will enable block ios reload if cios is d2x and ver >= 5
   'auto' is now the default
 * Removed detection of hermes ios installed with pimp my wii
   since the hashes were incorrect.

06-06-2011 cfg v69b2 (beta)
 * Use modmii cios identification for d2x version (by R2-D2199)
 * Detection of hermes cios v5.1 installed with pimp my wii (by xFede)

04-06-2011 cfg v69b1 (beta)
 * Support for ModMii cios identification (thanks XFlak, WiiPower)
 * Support for Korean games (by damysteryman)

03-06-2011 cfg v69a7 (alpha)
 * revert to ntfs-3g-2010.10.2 (libntfs-wii r4+r7)
 * Improved support for hdd with 4k sectors

03-06-2011 cfg v69a6 (alpha)
 * Improved support for hdd with 4k sectors

02-06-2011 cfg v69a5 (alpha)
 * Support for d2x v6 IOS reload block on FAT/NTFS/EXT2 fs.

02-06-2011 cfg v69a4 (alpha)
 * Fixed #001 error on hermes cios for fakesigned games (caused by
   Sam&Max fix which is now disabled on hermes cios, Thanks Wiimm)
 * Fixed We Dare (thanks airline38 and WiiPower)
 * Minor cleanups

28-05-2011 cfg v69a3 (alpha)
 * Really fixed d2x v5+ ios reload block on wbfs

28-05-2011 cfg v69a2 (alpha)
 * Fixed d2x v5+ ios reload block on wbfs (tnx WiiPower & davebaol)
   (Now Sam&Max fix is always enabled)

28-05-2011 cfg v69a (alpha)
 * detection of cios d2x v4 (tnx kamiro04 & FIX94)
 * d2x v5+ ios reload block on wbfs (tnx WiiPower & davebaol)
 * d2x v4+ return to channel method (tnx WiiPower & davebaol)
 * added value: return_to_channel = auto
   will try to detect the channel id from where the loader was started.
   (although some forwarders are not auto detected properly,
    but the official one by FIX94 is)

28-04-2011 cfg v68d (bugfix)
 * Fixed wrong banner sound playing (or hang or crash)
   when using WBFS and the new GUI menu.
 * Fixed missing text when a translation was missing
   (Now english is used if a translation is missing)

27-04-2011 cfg v68c (bugfix)
 * Fixed occasional corrupted cover in coverflow modes

24-04-2011 cfg v68 (release)
 * Fixed "press any button" after cover download
 * Fixed gamercard option in gui menu

22-04-2011 cfg v68b4 (beta)
 * Set custom buttons inactive when any window is opened
 * New theme options:
	gui_text_color_title
	gui_text_color_button
	gui_text_color_radio
	gui_text_color_checkbox
   Setting gui_text_color_menu will set the base gui menu color
   and all of the above color options too.
   Setting gui_text_color_button will also set radio and checkbox
   default value is same as gui_text_color_menu

21-04-2011 cfg v68b3 (beta)
 * Detection of cios d2x v4 beta 2
 * If pointer is outside a defined gui_cover_area then don't scroll
 * Fixed: wrong size of 2d&3d no cover image in gui menu game select
 * Fixed: admin unlock in gui mode to show full game list
 * Fixed: missing gui menu custom buttons after switching from console
 * Cleanups

19-04-2011 cfg v68b2 (beta)
 * New theme option:
   gui_button_NAME = X, Y, W, H, TextColor, image.png, Type, HoverZoom
   NAME can be: main settings quit style view sort filter favorites
   TextColor is same format as gui_text_color_menu
   (Seting TextColor to 0 will disable text on the button in case icons are used)
   Type: button or icon
   HoverZoom: 0-50 in %
   Paramteres after coordinates are optional. Default values:
   gui_button_NAME = X, Y, W, H, white/A0/44444444, button.png, button, 10
 * New theme option:
   gui_bar = 0, [1], top, bottom
   Will disable / enable gui bar or enable only top or bottom.
 * Minor fixes to gui admin unlock & left/right dpad with coverflow
 * Changed default gui menu color to white/A0/44444444
 * Removed [FRAG] note at startup

17-04-2011 cfg v68b (beta)
 * Translation fixes
 * Themable gui menu images:
   buttons: button.png checkbox.png radio.png
   windows: window.png page.png
   Images can also be placed in usb-loader base dir
   and if a theme doesn't provide it's own button.png
   then images from the base dir are used.
 * New theme options:
   gui_window_color_base = RRGGBBAA default: FFFFFF80
   gui_window_color_popup = RRGGBBAA default: FFFFFFB0

13-04-2011 cfg v68a4 (alpha)
 * Fixed Translation: Cover~~Front Cover~~Back
 * Fixed switch from gui to console mode
 * New themed options:
   gui_text_color_menu = COLOR / OUTLINE / SHADOW
   gui_text_color_info = COLOR / OUTLINE / SHADOW
   default:
   gui_text_color_menu = 6688FFFF / A0 / 44444444
   gui_text_color_info = white / A0
   each component can be "black", "white" or RRGGBBAA
   OUTLINE and SHADOW are optional
 * Bigger Start button
 * Minor GUI menu updates

11-04-2011 cfg v68a3 (alpha)
 * Translation updates
   New strings: Cover~~Front Cover~~Back Fav: Off Fav: On
 * Changed gui option:
   gui = 0, 1, 2, 3, [4], start
   gui = 0 : GUI disabled
   gui = 1 : GUI enabled, GUI Menu disabled, start in console mode
   gui = 2 : GUI enabled, GUI Menu disabled, start in GUI mode
   gui = 3 : GUI Menu enabled, start in console mode
   gui = 4 : GUI Menu enabled, start in GUI mode
   gui = start : same as gui = 4
 * Game dialog button changes:
   Pressing button A on cover will start the game
   To rotate the cover hold button 1
   To zoom use buttons +/-
   buttons UP/DOWN: change cover style
   buttons LEFT/RIGHT: prev/next game
 * Cover cache fixes

07-04-2011 cfg v68a2 (alpha)
 * Skip game start confirmation when started from gui menu.
 * When a game is selected and the wiimote is not pointing
   to the screen move the pointer to the start button so
   that pressing button A will start the game directly.
 * Always start with cover view in game dialog
   (previously it remembered the last state)
 * Changed gui option:
   gui = 0, 1, 2, [3], start
   gui = 0 : GUI disabled
   gui = 1 : GUI enabled but starts in console mode
   gui = 2 : GUI enabled and starts in GUI mode
   gui = 3 : GUI and GUI Menu enabled. Starts in GUI mode
   gui = start : same as gui = 3
 * Translatable GUI menu

06-04-2011 cfg v68a (alpha)
 * GUI menu

05-04-2011 cfg v67c (bugfix)
 * Fixed blurred coverflow in 50 Hz video mode
 * Fixed detection of missing covers in game options screen

04-04-2011 cfg v67 (release)
 * Fixed crash with intro=1 (Issue 123)
   Possibly also fixes problems with game forwarder channels
 * Other minor fixes and cleanups

03-04-2011 cfg v67b (beta)
 * Support for HQ covers (from Wiiflow, thanks to Hibern)
 * new option: cover_url_hq
   default: http://wiitdb.com/wiitdb/artwork/coverfullHQ/{CC}/{ID6}.png
 * File cache of compressed Full and HQ covers for faster loading
   (Saved to covers_path/cache)

22-03-2011 cfg v67a (alpha)
 * Improved cover cache - no more cover reloading
   after a theme or style change or cover download
 * Minor improvement in coverflow cover rendering
 * Minor adjustment of stick movement (nunchuck or classic)
 * Fixed slow movement in console mode game list
   when games are NTFS and hdd space info is enabled

15-03-2011 cfg v66c (bugfix)
 * Improved detection of cios d2x v3 (r21003) (thanks kamiro04)

13-03-2011 cfg v66 (release)
 * Version

11-03-2011 cfg v66b (beta)
 * Added detection of cios d2x v2,v3 (thanks kamiro04)

11-03-2011 cfg v66a (alpha)
 * Added detection of cios r21-d2x-v1 (thanks kamiro04)
 * Updated libfat to 1.0.9
 * ntfs-3g 2011.1.15 / libntfs-wii r7 (thanks Dimok)
 * new option: gui_pointer_scroll = 0, [1]
   disable/enable pointer scrolling of the game list
 * cleanups

25-01-2011 cfg v65 (release)
 * version

25-01-2011 cfg v65b8 (beta)
 * cios base detection for r17b

25-01-2011 cfg v65b7 (beta)
 * Fixed gui theme specified font_clock.png
 * cios base detection for r17

24-01-2011 cfg v65b6 (beta)
 * Fixed: sort = install-desc in config.txt
 * cios detection for base 57 r21+r19 modmii
 * Improved detection of hybrid modmii cios for non-249 slot

23-01-2011 cfg v65b5 (beta)
 * Fix modmii cios detection (again)

23-01-2011 cfg v65b4 (beta)
 * Fix modmii cios detection

23-01-2011 cfg v65b3 (beta)
 * Detection for modmii hermes cios v4, v5 (thanks FIX94)
 * Fixes for ios base detection

23-01-2011 cfg v65b2 (beta)
 * Detection for modmii ciosx rev19, 20, 21+19
   and hermes v4, v5, v5.1 (thanks FIX94)
 * Added "Show cIOS info" to global options menu

22-01-2011 cfg v65b (beta)
 * Added detection for modmii ciosx rev21 (thanks FIX94)
 * Print cios base and rev for slots 245-250
   in global options if button + is pressed
   and to saved debug.log
 * Word wrap game title in the game start confirmation screen
 * Minor cleanups

19-01-2011 cfg v65a (alpha)
 * Updated libfat to svn-4520 which includes FSINFO (by Dimok)
   FAT fsinfo stores the free space info to a designated sector
   speeding up the time it takes to print the free space.
   Previously the entire FAT table had to be scanned to get the
   free space which could take up to a couple of minutes.
   This was also the reason the option hide_hddinfo=1 is set
   so that by default the free space is not displayed .
 * Added a way to manually scan and sync the free space in fsinfo.
   Go to global options / partition selection and press 2
   Then all FAT filesystems will be shown and the free space for each
   By confirming with A the free space will be scanned again. If the
   scanned free space mathces fsinfo OK is displayed otherwise the
   correct free space, which is then stored to fsinfo.
   It is enough to do this once, after that the fsinfo should be kept
   in sync. (but using some other homebrew to write/delete data from
   FAT will make the info unsynced again, until all other apps are upgraded
   with the new libfat as well)
 * Minor change to usbstorage mem allocation (back to v63)
   (in case it fixes problems with v64 that were not in v63)

17-01-2011 cfg v64 (release)
 * Minor cosmetic fix in http error reporting

16-01-2011 cfg v64b6 (beta)
 * Fixed sort by install date

15-01-2011 cfg v64b5 (beta)
 * Improved USB timeout handling also when searching for config.txt on USB

15-01-2011 cfg v64b4 (beta)
 * Fixed BCA with hermes cios v5.1
 * Cleanups

14-01-2011 cfg v64b3 (beta)
 * Fixed theme download
 * Fixed wiitdb game info
 * Split (required) and [optional] controllers

13-01-2011 cfg v64b2 (beta)
 * Fixed hang in v64b when checking for updates

13-01-2011 cfg v64b (beta)
 * Fixed crash when downloading large cheat codes (Issue 100)
   Possibly fixes also the occasionally corrupted downloaded cover images
 * Added USBStorage_Deinit() to shutdown
 * Turn off wiimotes after they are idle for 5 minutes
 * Display supported controllers in wiitdb game info
 * Cleanups

12-01-2011 cfg v64a (alpha)
 * Support for ID4 wiitdb entries (wiiware)
 * Fixed unnecessary re-downloading of HQ full covers when they already exist
 * Better HTTP error reporting when downloading cover fails

10-01-2011 cfg v63 (release)
 * Minor cleanups
 * Full package changes: covers/2d, Languages -> languages

09-01-2011 cfg v63b6 (beta)
 * Fixed crash when missing wiitdb.zip
 * Translatable page indicator for wiitdb game info

09-01-2011 cfg v63b5 (beta)
 * Fixed coverflow slowdowns when using wiitdb
 * Improved switching between games in confirmation screen
   (disabled background mp3 while switching because of banner sounds)
 * Removed obsolete options cover_url_*_norm
 * More info in debug.log (mem, time)
 * Minor cleanups

08-01-2011 cfg v63b4 (beta)
 * Scrollable wiitdb game description (buttons UP/DOWN)
 * Switch to prev/next game in the confirmation screen (buttons LEFT/RIGHT)
 * Added DUTag to gamercard_url: http://tag.darkumbra.net/{KEY}.update={ID6}
 * Fixed partition=ask

04-01-2011 cfg v63b3 (beta)
 * Changed covers_path_2d default value to /usb-loader/covers/2d
   instead of /usb-loader/covers however both locations are still
   looked up so existing setups should not be affected. Downloading
   2d covers will save to /usb-loader/covers/2d only if it exists,
   otherwise /usb-loader/covers is used - same as before.
   If the option covers_path_2d is set manually in config.txt then
   it works same as before, the behaviour isn't changed.
 * Optimized sort by install date
 * Wiitdb optimisations
 * Added uDraw to filter by controller
 * Fixed WBFS partition on a 4k sector size drive
 * Minor cleanups

31-12-2010 cfg v63b2 (beta)
 * Fixed device init timeout handling

31-12-2010 cfg v63b (beta)
 * Improved device init timeout handling:
   If the device doesn't respond in 3 seconds one can
   try reloading IOS or exiting to HBC or sys menu
   timeout has been increased from 30 to 90 seconds
 * Minor cleanups

29-12-2010 cfg v63a3 (alpha)
 * Fixed using of ios slots 245 and 246
 * Fixed hang when game fails to load (at "Press any button to exit")

28-12-2010 cfg v63a2 (alpha)
 * Updated dip+frag plugin for ciosx r21
 * Added ciosx r21 base detection (Thanks FIX94)

28-12-2010 cfg v63a (alpha)
 * Added 2 more slots for waninkoko cios: 245, 246
 * Added saving of debug.log and ioshash.txt from global options screen
 * Fixed broken title in coverflow mode when no games are installed

22-12-2010 cfg v62 (release)
 * Updated libogc to 1.8.6

21-12-2010 cfg v62b3 (beta)
 * More translatable strings (languages, playlog)
 * Fixed: display of 3d cover when selecting a game in coverflow mode
   when a 2d and full cover are missing
 * Minor cleanups

19-12-2010 cfg v62b2 (beta)
 * Fixed gui_clock_pos alignment
 * Fixed title in grid mode when no game is selected
 * gui style change notification is now printed in title area
   the same way as button actions (profile change, ...)
 * Added option: gui_title_area = x, y, w, h
   default: 0,0,0,0 meaning, position depends on gui_title_top
   min w, h: 320, 10 note: h is not yet used
 * renamed option gui_pager_pos to gui_page_pos

19-12-2010 cfg v62b (beta)
 * fixed partition=auto for wbfs
 * new themable options:
 - gui_clock_pos = x, y
   default: -1,-1 meaning, title position is used
   if set clock is displayed all the time
 - gui_pager_pos = x, y
   default: -1,-1 meaning, title position is used
 * Translatable (Jabe & cambric request):
 - button names
 - partition types and header
 - video and language options
 - cover styles

18-12-2010 cfg v62a5 (alpha)
 * Fixed partition = auto
 * Better formatting of wiitdb info

18-12-2010 cfg v62a4 (alpha)
 * fixed ext2fs support
 * fixed error message about multiple wbfs partitions
   when using a second fat or ntfs or ext2fs part.

18-12-2010 cfg v62a3 (alpha)
 * fixed partition selection and crash from 62a2

17-12-2010 cfg v62a2 (alpha)
 * ext2fs support (Thanks to Dimok!)
 * new theme option: coverflow_reflection = color_top, color_bottom
   color is hex rgba, 0,0 will disable reflections
   default: coverflow_reflection = 666666FF, AAAAAA33 
 * new theme option: gui_cover_area = x, y, w, h
   default: gui_cover_area = 20, 24, 600, 408
   minimum accepted w, h: 480, 320
   enabling debug will draw the area rectangle
 * Improved coverflow to console transition
 * Updated libntfs (sync with wiiflow)
 * Updated grrlib: 4.0.0 -> 4.3.1
 * Updated intro 4 (smaller)

07-12-2010 cfg v62a (alpha)
 * Updated libs:
 - libfat 1.0.5 -> 1.0.7
 - jpeg 8a -> 8b
 - png 1.2.34 -> 1.4.4
 - zlib 1.2.4 -> 1.2.5
 * New default intro=4 : stripes themed (by abdias)

04-12-2010 cfg v61 (release)
 * In case a CODE DUMP happens the wii will reset in 60 seconds
   instead of waiting on the code dump screen
 * full package changes:
 - added a new default theme: stripes by abdias
 - removed themes: BlueMatrix, NXE, cfg_*
 - removed noimage_wide which is not used anymore
 - removed cfg61-222.dol (available as a separate download)
 In addition to cfg61-222.dol (with default ios 222-mload)
 there's now also:
 cfg61-compat.dol built with the old devkit 17+ogc 1.7.1
 cfg61-dbg.dol with debugging enabled by default

02-12-2010 cfg v61b9 (beta)
 * fixed startup with multiple wiimotes
   (libogc svn-4463 thanks to tueidj and tantric)
 * fixed hourglass image in coverflow mode

27-11-2010 cfg v61b8 (beta)
 * Allow to disable splitting of installed games on NTFS with fat_split_size = 0
 * If home=screenshot, don't disable it after screenshot is made in main menu

27-11-2010 cfg v61b7 (beta)
 * Fixed RAW fs / part.table detection

25-11-2010 cfg v61b6 (beta)
 * back to libogc 1.8.5 with CODBO fix (USB deinit, thanks to tueidj)

21-11-2010 cfg v61b5 (beta)
 * Fixed theme switching (Issue 69)
 * Increased max themes from 100 to 300 (Issue 60)
 * updated game count when downloading covers to start from 1 (Issue 92)
 * Removed boxart.rowdyruff.net from the list of cover urls

21-11-2010 cfg v61b4 (beta)
 * Use libogc 1.8.3 to fix COD:BO
 * Enable pointer control with any wiimote (but only one at a time -
   the one with the lowest number that points to screen is used)
 * more improvements to raw fs detection: in case the partition table is
   ambiguous - if it appears there is a raw fs and a valid part. table then
   make a decision based on device type: for sd assume raw, for usb assume p. table
 * Updated mp3 player (triple buffering from libogc)

19-11-2010 cfg v61b3 (beta)
 * Fixed .mod playing

18-11-2010 cfg v61t3 (debug test)
 * print more debug info, debug enabled by default
 * if home=screenshot then make a screenshot if home is being held for 1 second
   otherwise exit to hbc. So a short press on home will exit while holding home
   for 1 second will make a screenshot
 * possible fix for corrupted console text

cfg v61t2 (test)
 * init usb immediately after ios reload

cfg v61t1 (test)
 * libfat and libntfs build with devkit 17 and -Os

31-10-2010 cfg v61b2 (beta)
 * Updated libogc to 1.8.5
 * Fixed wiitdb synopsis for non-EN locale
 * Print wiitdb download url when updating
 * Reenabled loading a config file specified by args

30-10-2010 cfg v61b (beta)
 * Slightly faster startup time (by about 1-2 seconds)
   (optimized loading of config and wiitdb)
 * Selectable intro:
   intro=0 : black - only allowed when direct game launching
   intro=1 : black bg with program name (small)
   intro=2 : color image [default]
   intro=3 : grey image
   This option only works if set in meta.xml <arguments>
 * Improved NTFS related error messages: 
   - when starting games from NTFS compressed or encrypted files
   - when trying to install game or covers on ntfs with write disabled

27-10-2010 cfg v61a3 (alpha)
 * Upgraded devkitppc 17 to 22 (again)
 * Fixed devkitppc 22 and net related crashes (hopefully)
 * URL options will now accept also numeric IP address

23-10-2010 cfg v61a2 (alpha)
 * Reverted devkitppc from 22 back to 17 (libogc is still 1.8.4)
   This seems to fix the wiitdb and net related crashes in v61a
 * fixed: fat_install_dir = 3
 * better compatibility with some forwarders
   (ignore drive number (usb1:) in argv[0])

19-10-2010 cfg v61a (alpha)
 * Upgraded dev tools devkitppc 17 to 22 and libogc 1.7.1 to 1.8.4
 * Improved partition check for raw fs (v60t1 fix)
 * cios 222 shadow mload proper version (5.1) check
 * debug stuff:
  pressing + in global options screen will report:
  - devkitppc and libogc version
  - mem stats
  - startup timings
  option debug=16 will report game launch timings

12-09-2010 cfg v60 (release)
 * Fixed install on ntfs to .wbfs file type

12-09-2010 cfg v60b6 (beta)
 * Fixed and improved support for wbfs/fat/ntfs on RAW device

11-09-2010 cfg v60b5 (beta)
 * Fixed: partition = auto
   partition = auto is now the default value
   for both normal .dol and -222.dol

10-09-2010 cfg v60b4 (beta)
 * Fixed SSBB+ SD card
 * New option value: partition = auto
   Will select first valid from: WBFS1, FAT1, NTFS1
   FAT or NTFS partition will only be valid if the /wbfs folder exists

10-09-2010 cfg v60b3 (beta)
 * Fixed install to ISO on NTFS

09-09-2010 cfg v60b2 (beta)
 * New option value: install_partitions = iso
   On NTFS it creates an exact dump to an iso file
   On WBFS/FAT it will behave same as 1:1

08-09-2010 cfg v60b (beta)
 * Changed FS mountpoint handling:
   USB drive: 'usb:' is first FAT 'ntfs:' is first NTFS partition
   SD/SDHC card: 'sd:' is first FAT or NTFS partition if FAT is not found
   game: is a temporary mount for games on FAT or NTFS and will be any partition
   that is selected but is not already mounted as one of the above
   config.txt is now searched also on ntfs:/usb-loader/config.txt
   in addition to sd: and usb:

02-09-2010 cfg v60a (alpha)
 * NTFS write support
   new option: ntfs_write = [0], 1, norecover
   norecover will prevent mounting an uncleanly unmounted fs.
   (thanks for the updated libntfs go to Dimok and Miigotu)

01-09-2010 cfg v59 (release)
 * version change

31-08-2010 cfg v59b2 (beta)
 * dip+frag plugin for cios 249 updated to rev20
 * Changed the default value of: disable_dvd_patch = 1
   Since now the dvd slot check is handled properly by
   all cios the patch is disabled by default
 * Enable shadow mload on hermes cios v5.1

30-08-2010 cfg v59b (beta)
 * Loading games from SDHC for hermes cIOS
 * Update modifies existing meta.xml instead of replacing it (gannon)
   (So that any additional parameters or user edits will be retained,
   only version and date are updated)
 * Changed directory creation code to avoid errors on an incomplete path (Clipper)

27-08-2010 cfg v59a (alpha)
 * Improved support for Hermes cIOS v5.x
   Updated ehcmodule to v5 and new odip plugin
   Moved frag code from ehcmodule to odip
   Removed support for Hermes cIOS 1,2,3 and external modules
   (cios v4 is still supported)
   Games on SD/SDHC don't work with hermes cios for the moment.

26-08-2010 cfg v58 (release)
 * preliminary cios rev21 support

25-08-2010 cfg v58b2 (beta2)
 * Gamercard support (Clipper, Daileon)
 * New options: gamercard_url and gamercard_key
 - These options work like the cover_url options
   and support the =+ operator to add multiple sites/keys
 - Keys and URLs are matched up in order
 - If you set a key in the list to 0, or leave out trailing keys, the
   respective sites from the URLs will not be tried.
 - The tags {KEY} and {ID6} can be used in the URLs.
 - Defaults are for WiinnerTag and NCard in that order, but with blank keys:
   gamercard_url =  http://www.wiinnertag.com/wiinnertag_scripts/update_sign.php?key={KEY}&game_id={ID6}
   gamercard_url =+ http://www.messageboardchampion.com/ncard/API/?cmd=tdbupdate&key={KEY}&game={ID6}
   gamercard_key =

22-08-2010 cfg v58b (beta)
 * Added wiird setting to global options screen.
   (If the gecko is not connected the option is inactive)
 * Improved IOS base detection (from NeoGamma - by WiiPower)
 * New option: debug_gecko = [0],1,2,3
   write debug info to usb gecko

01-08-2010 cfg v58a (alpha)
 * Support multiple slots for Waninkoko's cios rev20: 247, 248, 249, 250
   Changed option: ios = 247, 248, [249], 222-mload, 223-mload,
   224-mload, 222-yal, 223-yal, 250

31-07-2010 cfg v57 (release)
 * Added video patching mode from Sneek (by Crediar)
   changed option: video_patch = [0], 1, all, sneek, sneek+all

30-07-2010 cfg v57b9 (beta9)
 * Added ID of 1.07 HBC to launch functions. (Clipper)
 * Allowed hex values for options that can specify a channel ID
   so the new HBC is supported.  Value for HBC 1.07 is AF1BF516. (Clipper)
 * Scrollable list of updates (oggzee)

24-07-2010 cfg v57b8 (beta8)
 * Added language selection to write_playlog (Clipper)
 * Improved playlog title detection: if the title for selected game language is empty,
   and playlog is enabled (but not set to a specific language) then try with english
   or the first language for which a title exists. (oggzee)
 * Fullscreen theme preview on button 1 (oggzee)
 * Download of theme preview can be cancelled by holding B (oggzee)
 * Moved theme preview images from themes/*.jpg to theme_previews/*.jpg (oggzee)
 * Optimizations of coverflow cover selection (usptactical)
   (realtime and compressed stencil buffer)

13-06-2010 cfg v57b7 (beta7)
 * Initial jpeglib support added (usptactical)
 * Added jpeg error handling: bad jpegs will be renamed 
   to filename.bad (usptactical)
 * Theme Preview images (usptactical)
   option: theme_previews = [1], 0
     Determines if preview images can be downloaded and 
     displayed.
   option: preview_coords = x,y,width,height
   option: wpreview_coords = x,y,width,height
     Set x/y to -1 to use Cover x/y
     Set width/height to 0 to use Cover height/width
 * Using select = random option in coverflow scrolls
   covers to next game (usptactical)
 * Bug fix for clicking on titles in theme download 
   menu. (Clipper)
   
08-06-2010 cfg v57b6 (beta6)
 * new option: select = [previous], start, middle, end,
                        most, least, random
   Selects which game is picked by default on startup.
   The new default is the previous game played (to get old
   operation, set select=start).
   'start', 'middle' and 'end' refer to position in the list.
   'most' and 'least' refer to number of plays (in Cfg).
   'random' selects a different game each time.
 * new button action: random
   Selects a different game at random from the current list
   To use, assign to a button, like button_Z=random
 * Title/alphabetical sorting made case insensitive (oggzee)
 * clear_patches save bug fixed
 * cheat bug fixed

25-05-2010 cfg v57b5 (beta5)
 * .wip patches now working
 * fix for .txt cheat detection (some cheats were incorrectly
   identified as editable)
 * New per-game option:
      clear_patches = [0],1,all
    When on (1) return_to_channel and the dvd check are disabled
    When 'all', then all .dol patches are disabled

20-05-2010 cfg v57b4 (beta4)
 * Patch for Prince of Persia
 * new option: disable_pop_patch = [0], 1
 * new option: disable_dvd_patch = [0], 1

20-05-2010 cfg v57b3 (beta3)
 * Fix for big theme downloading

16-05-2010 cfg v57b2 (beta2)
 * Theme downloading (via global options)
 * Themes provided by http://wii.spiffy360.com
 * new option: adult_themes = [0], 1
     Adult themes only shown for download if switched on

27-04-2010 cfg v57b (beta)
 * Added warnings for stubbed IOSes (gannon, Wiipower)
 * Changed warning for IOS249 <rev18+FAT32
   to make it more intuitive.

14-04-2010 cfg v57a (alpha)
 * After ripping a disc, you can push 1 to download images
 * new option: return_to_channel = [0], JODI, FDCL, ...
     Games will return to the selected channel ID
       e.g., JODI for HBC or a forwarder channel
       to reload Cfg.
     0 is the default Wii Menu operation
   Note: write_playlog won't work if you don't go back
     to the Wii Menu between games (only the last game
     played will be recognised in the log)
 * download_all_styles = 0 now downloads full covers
   if a coverflow GUI mode is selected.  2D images
   will be downloaded if the full download fails. 

06-04-2010 cfg v56c (bugfix)
 * Bug fix for alt dol disc plus (should fix Rampage problem)
 * Minor cosmetic improvements

18-03-2010 cfg v56 (release)

 * omit false DL warning

16-03-2010 cfg v56b2 (beta2)

 * Choosing "Disc" from alt dol menu will prompt for
   Disc Plus options on launch of game (choose for
   Metroid Prime Trilogy to be asked which to play)
 * Disc Plus bug fixes
 * Fixed sorting of long multibyte titles (oggzee)
  
14-03-2010 cfg v56b (beta)

 * Sam & Max fix restored for disc. (Clipper)
   Gets CSI: Deadly Intent working from disc.
 * Alt dol menu with scroll (Clipper)
 * Gui text outline fix
 * Minor intro update (pepxl)
 * Minor cleanups

13-03-2010 cfg v56a (alpha)

 * Alt.dol disc plus (from NeoGamma R8.RC4 by WiiPower / tueidj)
   .wdm files go to: sd:/usb-loader/GAMEID.wdm
 * New intro (by pepxl)

09-03-2010 cfg v55 (release)

 * No change

07-03-2010 cfg v55b3 (beta3)

 * Fixed occasional hang when trying to start
   games on FAT/NTFS with cios 249 rev19

07-03-2010 cfg v55b2 (beta2)

 * Games on SD/SDHC FAT/NTFS with cios 249 rev19

06-03-2010 cfg v55b (beta)

 * Fixed ios250 support
 * Updated 249 r18 dip with disc check fix
 * Print ios249 base ios (37,38,57,60) and mload version
 * Rename cfg-fat to cfg-222
 * Changed option default: hide_hddinfo=1 also for normal cfg.dol
 * Fixed magic word actions for the GUI
 * Changed intro
 * Cleanups

06-03-2010 cfg v55a3 (alpha)

 * 249 rev18 fat/ntfs other base than 38 fix
 * Bug fixes for "nothing" and button remap actions

05-03-2010 cfg v55a2x (experimental)

 * FAT/NTFS support for cios 249 rev18.
   NOTE: it overrides the dip plugin which adds frag support
   thanks to the mload capability in cios 249 rev18

04-03-2010 cfg v55a (alpha)

 * Added new values to the home and button_* options:
    priiloader - uses the Priiloader magic word "Daco" to go to Priiloader menu
    wii_menu - uses the Priiloader magic word "Pune" to go to Wii Menu
    Any other Priiloader magic word can be entered as text (no others exist yet)
    Any channel ID can be entered, e.g., FDCL for pepxl's forwarder

 Cool Usage Ideas:
   To go to Wii Menu when using Priiloader:
     home = wii_menu
     #OR
     home = Pune
   To implement a restart function after updates:
     home = FDCL #replace with your forwarder's ID
     button_H = exit
     # Because of the implementation of the options,
     # This makes the home button restart Cfg when
     # used in a menu like the global menu, and 
     # exit when used in the game list or GUI. 
   Implement multiple exit types and channel launchers:
     button_B = FDCL # reboot via forwarder
     button_1 = HATP # launch PAL Nintendo Channel
     button_2 = etc.


03-03-2010 cfg v54 (release)

 * Fixed: wiird = 2 (paused start) (by WiiPower)

28-02-2010 cfg v54b3 (beta3)

 * IOS 249 rev18 support

27-02-2010 cfg v54b2 (beta2)

 * Separate ocarina and hooktype options:
   ocarina = [0], 1
   hooktype = nohooks, [vbi], wiipad, gcpad, gxdraw, gxflush, ossleep, axframe
 * Print a warning if ocarina/wiird enabled but hooks can't be set
 * Print "Loading..." translation before freeing unifont

26-02-2010 cfg v54b (beta)

 * wiitdb: fallback to game language if configured language is not found
 * new option: wiird = [0], 1, 2
     1 = enable debugger
     2 = enable debugger and pause start
 * changed option: ocarina = [0], 1, vbi, wiipad, gcpad, gxdraw, gxflush,
     ossleep, axframe, nohooks
     ocarina = 1 is same as ocarina = vbi
 * support for wiitdb case color attribute (usptactical)

23-02-2010 cfg v54a2 (alpha2)

 * Per-game playlog writing now uses the game's language setting to determine 
   the title to use.
 * titles_url default now uses {DBL} instead of {CC}. 
 * Both titles_url and db_url can accept both {CC} and {DBL} tags now.

23-02-2010 cfg v54a (alpha)

 * Gecko OS 1.9 cheat engine aka Ocarina 2 aka Brawl+ support (by WiiPower)
 * Changed write_playlog to be a per-game option (Clipper)
 * fixed db_language AUTO setting and lang_to_cc function for Chinese languages. (Clipper)

23-02-2010 cfg v53 (release)

 * New option: load_unifont = [0], 1
   Specifies if unifont.dat should be loaded or not. unifont.dat contains
   all the unicode characters, required for Asian (CJK) language support
   so that translation or wiitdb info shows up correctly.
   Note: the Latin unicode set is already embedded into the loader,
   so to display German, French, Spanish, etc... unifont.dat is not needed
 * Renamed the included Chinese translation files according to standard:
   SChinese.lang -> ZH_CN.lang (Simplified Chinese)
   CHT.lang      -> ZH_TW.lang (Traditional Chinese)

22-02-2010 cfg v53b3 (beta3)

 * Fixed crash with cios222 v5
   (happened with this combination: normal cfg.dol
    with options: ios=222-mload & partition=wbfs)
 * Fixed update progress ... notification

21-02-2010 cfg v53b2 (beta2)

 * More fixes for handling of corrupted cover images (usptactical)
 * Minor translation updates
	
20-02-2010 cfg v53b (beta)

 * Better handling of corrupted cover images - they should not crash
   the loader anymore and will be renamed to filename.bad (usptactical)
 * Japanese / Chinese translation and wiitdb support (oggzee)
   A new font file is required for this: unifont.dat
 * Removed ISFS from playlog (Clipper)
 * Scroll option screens if the console size is too small (Clipper)
 * Removed wiiboxart from URLs. (Clipper)
 * print cover download url and progress (oggzee)
 * force fat freespace update when installing (oggzee)

16-02-2010 cfg v53a (alpha)

 * cIOS 222/223/224 v5 support
   Note: only use 222 for loader, 223 and 224 will freeze if used for loader,
   however 223/224 work fine for games. That means, don't put ios=223-mload in
   config.txt, but it's ok if it is set for a specific game in options screen.
 * New option value for ios: 224-mload
 * Support for HDDs with 4k sectors (WBFS partition only)
 * Fixed option: home=hbc

13-02-2010 cfg v52 (release)

 * Left/Right hold repeat in console
 * GUI displays messages for sort, profile and theme switch.
 * Minor cleanups

12-02-2010 cfg v52b5 (beta5)

 * Fixed handling of multiline strings in .lang files
 * Minor translation updates

11-02-2010 cfg v52b4 (beta4)

 * New actions for buttons: sort (switch sort type), filter (filter menu) (Clipper)
 * Button actions sort, profile and theme will display a message in the console (Clipper)
 * Holding any of the buttons in button_other in the GUI will work for menu_unlock (Clipper)
 * Fixed: title length 3 from folder names
 * Fixed: WiiTDB update crash
 * Handle &amp; etc. in wiitdb titles
 * fat_install_dir = 3 will use layout: /wbfs/Title [ID].wbfs
 * new option: fs_install_layout is an alias for fat_install_dir
 * Minor cleanups


09-02-2010 cfg v52b3 (beta3)
 * Button remapping options (Dr. Clipper)
   See below for information.
 * Previous home option is now a theme option with overrides
 * Reversion of boot disc to cIOS method (for real this time)
 * Fix for switching between NTFS partitions (oggzee)
 * Various translation and menu alignment fixes (oggzee)
 * Support for new filenames on FAT/NTFS: (oggzee)
   /wbfs/TITLE [GAMEID].wbfs  or  /wbfs/TITLE [GAMEID].iso
 * option: db_ignore_titles = [0], 1
     Set this option to ignore titles from the database

 About button remapping: 
  Firstly, the guitar default mappings have changed slightly.
    The new mappings are as follows:
    RED = A; GREEN = B; YELLOW = X; BLUE = Y; ORANGE = Z.

  Each of the following buttons can now have its own action:
  B, -, +, 1, 2, Home, X, Y, Z, C, L & R.

  These actions are valid for the console game list and the GUI 
  mode only.  For options that affect the menus, see below.
  The new options for this type of mapping are all theme options
  with config.txt overrides and are as follows:	
   option: button_B = [gui], <other actions>
           button_- = [main_menu], <other actions>
           button_+ = [install], <other actions>
           button_H = [reboot], <other actions>
           button_1 = [options], <other actions>
           button_2 = [favorites], <other actions>
           button_X = A, B, 1, [2], H, -, +, <action>
           button_Y = A, B, [1], 2, H, -, +, <action>
           button_Z = A, [B], 1, 2, H, -, +, <action>
           button_C = [A], B, 1, 2, H, -, +, <action>
           button_L = A, B, 1, 2, H, [-], +, <action>
           button_R = A, B, 1, 2, H, -, [+], <action>
  These buttons can be mapped to any of the following actions:
     nothing    # does nothing
     options    # access game options
     gui        # switch to/from GUI
     reboot     # reboot to system menu
     exit       # exit to launching app
     hbc        # exit to HBC
     screenshot # take a screenshot
     install    # install a game
     remove     # remove a game
     main_menu  # access main menu 
     global_ops # access global options menu
     favorites  # toggle favorites view
     boot_game  # boot a game from the drive
     boot_disc  # boot a game from disc
     theme      # switch to next theme
     profile    # switch to next profile
     unlock     # access the unlock password dialog immediately
  As shown, X, Y, Z, C, L & R can also be optionally targetted to 
  emulate one of the buttons on the Wiimote (A, B, 1, 2, -, +, Home).
  If used this way, this emulation will also work in menus.

  As stated, the other options allow you to select the default
  action in the game list and GUI mode only.  The menus can be
  remapped by specifying which buttons affect which commands.
  These options take a commas separated list of button names from the
  following list:
    B, 1, 2, -, M, Minus, +, P, Plus, H, Home, X, Y, Z, C, L, R

  The following are the mappable commands.  All the options are theme
  options with overrides in config.txt.

   option: button_cancel = [B], <comma separated list of buttons>
     Set which button(s) will act as the back button in menus

   option: button_exit = [Home], <comma separated list of buttons>
     Set which button(s) will perform the 'home' action in menus

   option: button_other = [1], <comma separated list of buttons>
     Set which button(s) will perform the other or alternate action in menus
     This covers switching between options and global options, choosing to
     download BCA during install, choosing to ignore meta.xml during upgrade etc.

   option: button_save = [2], <comma separated list of buttons>
     Set which button(s) will perform the save action in menus

 EXAMPLES:
   To switch buttons B & 1 around so that 1 operates as GUI while
   B operates as back:
     button_B = options
     button_1 = gui
     button_other = B
     button_cancel = 1
   To make both the L and R buttons on a GameCube controller
   operate as back buttons in the menus in addition to B:
     button_cancel = B, L, R
   Plug in the Classic controller and you can have any twelve
   different actions available at once (with A being boot_game):
     button_B = gui
     button_1 = options
     button_2 = favorites
     button_- = profile
     button_+ = theme
     button_H = exit
     button_L = remove
     button_R = install
     button_X = main_menu
     button_Y = global_ops
     button_Z = boot_disc

06-02-2010 cfg v52b2 (beta2)
 * File custom-titles.txt in the base directory is searched 
   for game titles.
 * Titles are extracted from wiitdb.zip but can be overridden 
   with either titles file.
 * The titles precedence (highest to lowest) is as follows:
     - custom-titles.txt
     - titles.txt
     - wiitdb.zip
     - directory name (FAT & NTFS only)
     - game image
 * When saving global options, the saved settings are listed.
 * Console color fixes (Dr. Clipper)
 * Play time logging to message board (marc_max & Dr. Clipper)
     When enabled, this option will put the correct title
     and play time into the Wii Message Board log and will
     also be read by the Nintendo Channel.  However, this will
     usually fail if you skip the Wii Menu via BootMii or 
     Priiloader autoboot.
 * option: write_playlog = [0],1 
     Note, it is disabled by default as this fix changes your
     Wii's NAND and cannot be used via autoboot methods.

31-01-2010 cfg v52b (beta)
 * Gamecube disc loading
   Just like Wii discs, only original discs supported!
 * Wii disc loading now uses the disc specified IOS.
   This should increase game compatibility.
 * Console font outline and shadow fix by Dr. Clipper
 * Many translatable strings have been improved.
 * Cover URLs updated

23-01-2010 cfg v52a2 (alpha 2)
 * Fixed options.
   option: language_path has been removed.
   The path is now fixed at /usb-loader/languages/
   option: language has been changed to translation to 
   prevent conflicts with the game language setting.
   option: translation = [AUTO], EN, custom, etc

23-01-2010 cfg v52a (alpha)
 * Translation files now supported.
   option: language_path = path to language files
   Default: USBLOADER_PATH/languages
   option: language = filename without extension
   Default: Current the wii language from the following list
            JA, EN, DE, FR, ES, IT, NL, ZH, ZH, KO
 * Fixed crash issue if booting from disc failed
 * Database can now be named wiitdb.zip.
   The old naming scheme is still supported however.

17-01-2010 cfg v51 (release)
 * New Sort: last play date
   option: sort = play_date
 * Removed empty line from game list when showing database info
 * Secondary sort using titles added. Lists should be consistent
   when there are matching values now

13-01-2010 cfg v51b3 (beta3)
 * Fixed the ambiguity with the game dir layouts (ID_TITLE or TITLE [ID])
 * fat_install_dir = 2 will use the new layout (TITLE [ID]) when
   installing
 * Removed redundant options from main menu.
 * Cleaned up the sort and filter menus.
   Improved sort menu. Ascending / descending options for current 
   sort are remembered.
 * Color of database info now changed. 
 * Install and disc boot menus will show [?] cover before a disc is 
   read, and game cover for disc if found.

11-01-2010 cfg v51b2 (beta2)
 * More bug fixes
   Loader no longer crashes when trying to sort or filter without
   a database.
   Accented characters now show up in the synopsis.
   Display of synopsis cleaned up and improved.
   Entities now converted in the synopsis. (&quot;, etc)
   Main menu will respect the disable_options configuration.
   sort=play_count now works properly without reloading the game list.

10-01-2010 cfg v51b (beta)
 * Minor bug fixes
   Loader will not wait for a button press in the event a database
   is not found.
   Disc boot menu will show the proper database information.
 * Changed db_url option and db_language option slightly
     option: db_url = [http://wiitdb.com/wiitdb.zip?LANG={DBL}]
     {DBL} will be replaced by the db_language value
     option: db_language = [AUTO], EN, JA, German, etc
 * option: "-asc" is no longer necessary to specify a sort as ascending.
 * db_show_info no longer hides the hdd info or footer in the console.
 * Added more game directory layouts: (by oggzee)
   /wbfs/TITLE_[GAMEID]/GAMEID.wbfs
   /wbfs/TITLE [GAMEID]/GAMEID.wbfs
   /wbfs/TITLE[GAMEID]/GAMEID.wbfs
   When loading games from FAT or NTFS
 * Added {PUB} to cover url options.
   {PUB} will be replaced by the last two characters of the ID
   (the publisher)
   This can be used to do things like forcing NTSC covers for 
   PAL games by replacing {CC} with US and {ID6} with {ID3}E{PUB}

09-01-2010 cfg v51a (alpha) (gannon)
 * Wiitdb support. Can be downloaded inside the loader on the
   global options screen.
 * Enhanced nunchuk support: C mapped to A, Z mapped to B
 * Filtering of games based on certain criteria
 * Sorting of games based on certain criteria
 * Gameplay history
 * Disc Loading
 * New: Main menu accessible by pressing - or going to the
   global options screen.
   Disc loading, sorting, filtering, and more options are located here.
 * option: db_url = [http://wiitdb.com/wiitdb.zip?LANG={db_language}]
   URL to download database from.
 * option: db_language = [Console Language], EN, JA, German, etc
   Language to use for the database. If invalid or not able to be
   displayed by the loader this will default to English.
   Both country codes (EN) and languages (English) are valid.
 * option: db_show_info = [1], 0
   Show info loaded from the database.
 * option: write_playstats = [1], 0
   Write to the play history file.
 * option: sort = [title-asc], etc
   Change the default sorting method. Default is Title Ascending.

   Valid sort options:
     "title"         => Title
     "players"       => Number of Players
     "online_players"=> Number of Online Players
     "publisher"     => Publisher
     "developer"     => Developer
     "release"       => Release Date
     "play_count"    => Play Count
     "install"       => Install Date
                        (This will only work with FAT or NTFS drives)
   To use ascending add "-asc" to the option.
     ie: sort = players-asc
   
   To use descending add "-desc" to the option.
     ie: sort = players-desc


21-12-2009 cfg v50c (bugfix)

 * Fixed starting games from SD Card with FAT or NTFS

16-12-2009 cfg v50 (release)

 * Optimizations for highly fragmented files (either fat or ntfs)

15-12-2009 cfg v50b2 (beta2)

 * Fixed crash when using flat /wbfs file layout without subdirectories
 * Fixed crashes when starting HBC forwarder discs
   And added safety checks of memory regions when loading disc 
 * Raised number of fragments limit to 20000
 * Properly identify dual-layer iso
 * Removed obsolete ehcmodules for IOS 222 revisions 2 and 3
   External ehcmodules for these versions are still supported
 * Use the new ehcmodule with fat/ntfs support also for wbfs
   (but still uses wbfs mode for wbfs partition)
   Can be overriden using an external ehcmodule4
   External module for fat/wbfs has been renamed
   from ehcmodule_fat.elf to ehcmodule_frag.elf
 * In case the new fragments method fails for any reason for FAT
   it will fallback to the old method
 * Other cleanups

14-12-2009 cfg v50b (beta)

 * .iso files on NTFS support
   The file name layout is the same as for .wbfs files:
   /wbfs/gameid.iso or /wbfs/gameid_title/gameid.iso
 * Fixes and cleanups for NTFS support (fixed ntfs getf -1 error)
 * option: partition=ntfs1 accepted

 About NTFS support: 

    FAT support in ehcmodule has been rewritten with a new
    generic wii disc emulation system that is:
    - light weight / zero overhead
    - filesystem independent
    - fileformat independent
    It works by supplying the ehcmodule with a list of sector fragments
    that specify the location of data using direct sector addressing.
    To see the list of fragments one can use debug=1 and they will
    be printed out before starting the game.
    The number of fragments if limited to 5000, that number is also
    the max theoretical number of fragments on a wbfs partition (actually
    4600, for a dual layer disc with a 2mb wbfs block size). In normal
    conditions the number of fragments should be a lot lower most commonly
    just a single big block. Fragments are used to describe both physical
    address on hdd and virtual adress on wii disc so if a .wbfs file is used
    the list will be composed of 3 fragments - disc header, update partition
    and game partition.
    libntfs however doesn't seem stable enough for write access at the moment,
    so the ntfs partition is mounted read-only meaning install and remove can't
    be done from inside the loader for now.

 Credits: WiiPower for libntfs modification which returns the list of fragments.

12-12-2009 cfg v50a (alpha)

 * Fix for PeppaPig (from NeoGamma by WiiPower)
 * Fixes and cleanups for NTFS

10-12-2009 cfg v50x (experimental)

 * Rewritten FAT support in ehcmodule with a generic system
 * NTFS support
 * Improved gui speed with large number of games
   (most noticable in grid, flow and flow-z gui styles)
 * Print on the intro screen if the ios is reloaded a second time
   (in case the setting from config.txt is different from default)
 * The -fat version 'simple' option does not change 'hide_hddinfo'
 * Changed WBFS ERROR: read error while installing a game to a WARNING.
   Note: the read error check has been introduced in v47, all previous
   versions including the original loader 1.5 and all other loaders
   silently ignore it.
 * Changed default value of install_partition=only_game
   To avoid errors caused by modchips when trying to copy the update partition.
 * Minor cosmetic changes to cover download when trying different urls.

08-12-2009 cfg v49 (release)

 * Fixed install game on SD/FAT
 * Override [w]covers_size theme option with config.txt
 * simple=x will always set hide_hddinfo when using -fat version
 * Only one "#GAMEID" string inside binary - for direct starting

07-12-2009 cfg v49b2 (beta2)

 * Improved speed of loading game list when using FAT and /wbfs/id_title/ subdirs
 * Changed default: fat_install_dir=1
 * When downloading titles.txt and wii region is JA or KO force EN in titles_url {CC}
 * Allow specifying alt_dol=name (on disc) when using direct start
 * Accept GAMEID without # as argument for direct start (RHAP01 instead of #RHAP01)
 * Override some theme options in base config.txt.
   The options that can be overriden are those that don't
   have a major effect on the theme looks and layout:
   - hide_header
   - hide_hddinfo
   - hide_footer
   - buttons
   - simple
   - cover_style
   - cursor
   - menu_plus
   - gui_text_*
   - gui_text2_*
   - gui_title_top
 * Save cfg loader version when saving gamelist.txt

03-12-2009 cfg v49b (beta)

 * Added BCA dump to file from install menu
   (Press + to install and then press 1 to dump BCA)

01-12-2009 cfg v49a (alpha)

 * Games on SDHC with IOS 222/223 for both FAT or WBFS partition
 * Games in subdirs on FAT: /wbfs/GAMEID_TITLE/GAMEID.wbfs
   option: fat_install_dir = [0], 1
 * Rename old boot.dol to boot.dol.bak when upgrading
 * If the loader is used to start a game directly
   (from a channel created with crap or similar tools)
   and option: intro=0 is specified then no intro
   and no progress is displayed while the game is being loaded
 * Support for .wip game patches (by WiiPower)
   Loaded from: sd:/usb-loader/GAMEID.wip (text format)
 * Support for BCA data (by Hermes)
   Loaded from: sd:/usb-loader/GAMEID.bca (binary data of size 64 bytes)
   (updated dip plugin from uloader 3.2)
 * option: disable_nsmb_patch=[0],1 will disable the builtin nsmb patches
   (in case someone wants to use/test the external .wip patch or .bca data)
 * Enable WiiRD if usb gecko is connected and ocarina is enabled
   even if not codes are found (by Rfrf)
 * Ocarina url fix: /R/ID6 instead of /ID1/ID6 (for SNM*)


21-11-2009 cfg v48 (release)

 * External ehcmodule improvements (by WiiZ)
 * Fixed titles.txt with custom game ids (proper ID6 lookup)

20-11-2009 cfg v48b2 (beta2)

 * FAT loading speed optimisations
 * Support for 4gb .wbfs files on FAT - fixed the 2gb limit
 * Changed the default split size when installing to 4gb-32kb
 * Option: fat_split_size = [4], 2
 * Benchmark mode with debug = 8 and start or install a game
 * Allow space in profile name, so the names must be separated by a comma (,)
 * Raised max favorites to 100
 * Removed obsolete cover sites: gateflorida.com, awiibit.com
 * Cleanups

16-11-2009 cfg v48b (beta)

 * NSMB NTSC patch
 * Make updating meta.xml optional by pressing button 1 in the
   online update screen to skip it.
 * Specifying titles url allows the use of {CC} tag for the country code.
   changed default: titles_url = http://wiitdb.com/titles.txt?LANG={CC}
 * Presing button A on global options screen works too (same as right)

15-11-2009 cfg v48a (alpha)

 * Profiles - aka multiple favorite groups
   option: profile_names = [default], name2, name3,...
   (max profiles:10, max profile name length: 16)
   Profiles can be added, removed and reordered with this option.
   But if you want to rename it, you will need to change the profile
   name also in settings.cfg otherwise it will be considered as a new
   name and the old one will be forgotten next time you save settings.
   Profiles can be switched in the global options screen. Changing a
   game favorite setting is done as usual in the game options screen.
   option: profile = name will specify the default profile to use
   saving global settings will also save which profile is used
 * Minor fix to favorites switching in console mode
 * Update /apps/.../meta.xml when downloading an update.
   So that the correct version is visible in HBC
 * Added 'Update titles.txt' to global options screen.
   option: titles_url = [http://wiitdb.com/titles.txt]
   Can be changed to a localized version like this:
   titles_url = http://wiitdb.com/titles.txt?LANG=FR

14-11-2009 cfg v47 (release)

 * Minor coverflow fixes
 * Cleanups

12-11-2009 cfg v47b3 (beta3)

 * Support for guitar controller in the loader (by gannon)
 * More banner sound fixes (opening.bnr case)
 * Changed the default value of the option: hide_hddinfo=1 But only for the -fat
   variant, because checking the disc free space on fat takes a few moments.
 * 4GB partition offset fixes
 * Increased the updates list length to 8
 * Minor coverflow fixes

09-11-2009 cfg v47b2 (beta2)

 * NSMB main.dol patch by WiiPower
 * minor coverflow gui fixes

09-11-2009 cfg v47b (beta)

 * GC and classic controller and nunchuk joystick support (by gannon)
 * option: install_partitions = 1:1
   disable scrubbing when installing, except for the last 256kb which are
   stil scrubbed because of the wbfs block size not aligned to wii disc size
 * Fixed banner sounds with games that have upper case OPENING.BNR
   (Thanks to RolloS60 from wiicoverflow)
 * Cleanups

06-11-2009 cfg v47a (alpha)

 * Better compatibility with weird WBFS partition setups:
 - WBFS on RAW disk device, without a partition table
   With such a setup the partition table will look like this:
    P# Size(GB) Type Mount Used
    -----------------------------
    RAW  500.00 WBFS1      [USED]

 - WBFS on EXTENDED partition. Normally a filesystem should
   reside on either a primary or logical partition, never on
   an extended partition. Extended is just a container of logical
   partitions. However older version allowed this setup and so
   the support for it is back. The partition table will look like:
    P# Size(GB) Type Mount Used
    -----------------------------
    P#1  500.00 EXTEND/WBFS1 [USED]
   And the loader will also let you fix the partition table by
   pressing 1, which will change the partition type from EXTEND to
   a known data type (0x0b, which is also used for fat)

01-11-2009 cfg v46 (release)

 * Support for direct launching of games from fat
   (Useful for game channel launching, using loadstructor or similar tool)
   To specify direct loading from fat, the parameter has to be in form: #GAMEID-X
   Where X is 0-3 for wbfs and 4-9 for fat partitions (4 means first fat partition)
 * Automatically switch to ios222 in case ios249 is used for starting games from fat
 * Only allow IOS 222 and 223 in game options if fat partition is selected
 * Improved drawing speed in some coverflow gui modes
 * Cleanups

30-10-2009 cfg v46b (beta)

 * Added option: partition = [WBFS1], ..., WBFS4, FAT1, ..., FAT9, ask
 * Saving global settings will also save current selected partition
 * Increased fat cache size in IOS module
 * Load FAT module in IOS early in case config has: 
   ios=222-mload (or ios=223-mload) and partition=FAT*
   So that IOS does not need to be reloaded another time before
   starting the game from a FAT partition
 * Added indication in global options if [FAT] module is loaded in IOS
 * Cleanups

26-10-2009 cfg v46a2 (alpha2)

 * Faster loading of game list from a fat partition - should be instant now.
   The only thing that has a slight delay is showing the hdd free space
   in console mode game list (if it is enabled)
 * Create game info file after installing on a FAT partition named:
   usb:/wbfs/GAMEID_TITLE.txt This makes it easier to tell the titles
   of installed games when using a filesystem explorer
 * Rephrased the warning message when no WBFS partition found
 * Allow to select partition if no wbfs partition is found and disable_format is on.
 * Minor cleanups in partition selection

25-10-2009 cfg v46a (alpha)

 * Loading games from a FAT32 partitions. The game file has to
   be located and named like this: usb:/wbfs/GAMEID.wbfs

16-10-2009 cfg v45 (release)

 * full package, no code changes

12-10-2009 cfg v45b2 (beta2)

 * Fixed multiple WBFS partition support for drives larger than 1TB
 * Fixed "DVD in drive check" by patching the game (thanks giantpune)
   (only if IOS249 before rev12 or IOS222/223)

12-10-2009 cfg v45b (beta)

 * Updated ehcmodule for IOS222-mload from uLoader 3.1
 * Fixed disable_format option
 * Show game info in options screen while scanning for alt.dols
 * More IOS related warning notes: (thanks WiiPower)
 - cIOS before rev14: possible 001 error source on new games (Wii Fit Plus)
 - cIOS before rev13: need a disc in the drive
 - cIOS before rev10: no sd/sdhc support
 - cIOS before rev9: no usb support
 * Changed the default for: download_id_len = [6]
 * Changed IOS order - moved *-yal after *-mload
 * Show version on intro screen
 * Partition selection screen improvements
 * The usual minor cleanups

11-10-2009 cfg v45a (alpha)

 * Multiple WBFS partitions support (from uLoader)
   Limitation: max 4 WBFS partitions on same drive supported by ios 222/223
 * Print game size, dual-layer and wbfs free space in install screen
 * Save gamelist.txt when saving settings in global options screen
 * option: home = hbc will exit to Homebrew Channel (thanks to giantpune)
   (similar to home = exit, but might work better with forwarders)
 * Stability fixes:
 - Corrupted cover images should no longer crash the loader
 - Fixed hdd spin down/up at startup for some drive models (since v43, tnx Narolez)
 - Don't print error if opening.bnr not found
 - mp3 playing stability fixes (fixes stuttering and slowdowns in rare occasions)
 - fixed the few notes of music between banner sound and game start
 - minor cleanups
 * Added CIOS related warning notices: (thanks Clipper)
 - CIOS249 rev13 : unsupported "return to menu" to exit game
 - CIOS249 rev14 : unsupported dual layer (start, install)
 - CIOS249 : unsupported multiple WBFS
 - CIOS222 : unsupported SDHC WBFS
 * New intro screen (by Milcoi)

01-10-2009 cfg v44 (release)

 * Full package
 * Updated resources/fonts* with unicode and clock fonts
 * Updated titles.txt (from wiitdb.com)
 * Added localized resources/titles-XX.txt (EN, FR, DE, ES, IT, NL, PT)

28-09-2009 cfg v44b6 (beta6)

 * More banner sound fixes (again)

28-09-2009 cfg v44b5 (beta5)

 * More banner sound fixes (thanks to Dr.Clipper)

28-09-2009 cfg v44b4 (beta4)

 * More banner sound fixes
   all 3 audio formats are implemented, so the few rare
   games that previously didn't play should work now.
 * Clock font can be changed with font_clock.png
   (either from theme or base dir)

27-09-2009 cfg v44b3 (beta3)

 * Banner sound fixes

27-09-2009 cfg v44b2 (beta2)

 * Play banner sound in the game start confirmation screen
 * option: clock_style=[24],12,12am,0

26-09-2009 cfg v44b (beta)

 * Updated ehcmodule for CIOS 222/223 rev4 to uloader 3.0C
 * Fixed ios argument from forwarder
 * Option: sort_ignore=A,An,The
   (sort_ignore=0 for old sort method)
 * Show clock in GUI mode if wiimote doesn't point at screen for 5 seconds
 * Skip gui transition at start
 * changed default: gui_text_outline=FF
 * Minor unicode fixes

22-09-2009 cfg v44a (alpha)

 * Improved coverflow gui mode:
 - better transitions between modes and console
 - fluid movement of covers

 * Latin unicode support for titles.txt (EN,ES,FR,DE,PT,IT)
   (not supported: Japanese, Korean or Chinese)
   New font name: font_uni.png (can be created with the Confugrator)
   Localized titles.txt can be downloaded from:
   http://wiitdb.com/titles.txt?LANG=FR

 * parse cmd line for ios=... before ios reload
   (used by forwarders)

 * Changed message for device init error to:
   Make sure USB port 0 is used!
   (The one nearest to the edge)

 * Scale intro to fullscreen in 576i mode


16-09-2009 cfg v43 (release)

 * minor code cleanups

13-09-2009 cfg v43b (beta)

 * Improved antialiasing in coverflow (sharper)
 * Reorganized game options screen:
 - Allow to change saved options
 - Unsaved changes are private per-game, not global
 - Moved alt.dol selection to options menu
 * Minor gui fixes: style switching, screen scale glitch

09-09-2009 cfg v42c (bugfix)

 * Fixed green bar when loading game with a different cios (222...)

08-09-2009 cfg v42 (release)

 * Fixed scrolling glitch in options screen
   with themes that have small console size
 * Modified intro screen (based on Milcoi design)
 * Moved larger themes to a separate package

07-09-2009 cfg v42b2 (beta2)

 * Fixed coverflow antialiasing in PAL 576i (50Hz) mode
 * Fixed occasional flashing in options screen
 * Intro screen (thanks to Milcoi)

03-09-2009 cfg v42b (beta)

 * Antialiasing in CoverFlow GUI modes.
   Can be tuned with option: gui_antialias = [4] {0-4}
 * cios 222/223 rev4 support:
 - using new dip_plugin and ehcmodule from uloader 3.0B
 - external ehcmodule for rev4 has to be named: ehcmodule4.elf
 - note: rev2&3 modules are still integrated and used appropriately.
 * Changed url tag CC value for portugal/brasil from PO to PT
 * Removed obsolete options: cover_url_*_wide, download_wide
 * Show 6 letter ID in game options screen
 * Better support for custom games in titles.txt - use full 6 letter IDs
   if available, otherwise 4 letter IDs still work same as before.
   Note: 6 letter ID in titles.txt was supported before but just 4 were used.
 * Show a note in the global options screen if there are multiple config.txt
   files used (one in base and another in apps/... dir)
 * Fixed occasional crash with the combination of
   options: video = game, video_patch = all
 * Minor fixes (thanks to wiimm)

01-09-2009 cfg v42a (alpha)

 * Ability to force progressive mode: (from NeoGamma)
 - Split 'patch' video mode to a separate option:
   video_patch = [0], 1, all
 - 'video_patch = all' will force all modes to the selected video mode.
   This will also force progressive / interlaced mode, depending on what
   is configured in wii settings. This can be used for example to force
   progressive mode if the game will otherwise use interlaced mode (MPT)
 - The equivalent of the old video = patch is now:
     video = system
     video_patch = 1
   This will patch NTSC -> PAL modes if console is PAL
   and PAL -> NTSC modes if console is NTSC
   This will not change interlaced / progressive mode

 * Allow wiird to work if ocarina is enabled and no codes are found.


21-08-2009 cfg v41 (release)

 * Fixed minor glitch with resized overlays
 * Increased hide game limit to 500
 * Updated all themes in the package to use overlays
   (while still being backward compatible)
 * New themes included: (all using overlays)
   - NXE_transparent by Blue-K
   - Nature 3D by DonTlaloc
   - Black Cat by Milcoi

18-08-2009 cfg v41b (beta)

 * Streamlined steps in game install screen
 * Fixed green screen before starting game (from cloader)

17-08-2009 cfg v41a (alpha)

 * Improved theming:

 * Background overlay support
   additional images can be supplied that will be overlaid over the background:
   - bg_overlay.png or bg_overlay_w.png for console background
   - bg_gui_over.png or bg_gui_over_w.png for gui background
   If *_w.png variant is not found then the normal is used.
   New options:
   - background_base
   - wbackground_base
   - background_gui
   - wbackground_gui
   Used to specify the background base over which the overlays are applied.

 * background width can be larger than 640 and will be either
   - scaled if widescreen
   - cropped if not widescreen
   down to 640x480. Note: height must still be 480.

 * Option layout no longer re-sets the covers_size and wcovers_size
   So that a cover_style or [w]cover_size in front of layout works as expected.


09-08-2009 cfg v40 (release)

 * Increased timeout for entering password to 30 seconds
 * Renamed option admin_lock to admin_unlock as that better describes
   what it does - it allows you to unlock admin functionality
 * Fixed option unlock_password

08-08-2009 cfg v40b3 (beta3)

 * uLoader cIOS 222/223 rev3 support
 - both rev2 and rev3 are supported
 - ehcmodule for rev2 is updated to uloader 2.5
 - ehcmodule for rev3 is from uloader 2.8D
 - external ehcmodule for rev2 has to be named: ehcmodule.elf
 - external ehcmodule for rev3 has to be named: ehcmodule3.elf
   
 * Minor GUI speed optimizations of rendering and cover loading

 * Admin unlock by password

 * Hide games from settings screen

 * Added new config option (admin_lock = 0,[1]) for admin locking (i.e. Parental Mode).
   When this setting is enabled, it will allow all screens normally locked by the
   simple or disable_* settings to be unlocked via a "secret" wiimote button combination.
   In addition: when unlocked, any covers that are hidden with the hide_game option will
   be displayed.  To access the unlock screen, hold the 1 button down for 5-10 seconds
   and the screen will appear.  After you see the text "Enter Code:", press the wiimote
   buttons in the correct order.  If you were successful, the word SUCCESS will appear
   on the screen.  Otherwise the word LOCKED! will appear.  The unlock screen has a 15
   second timeout limit so if an incorrect (or no) password is entered, it will
   automatically lock.  To set the lock back on with the original settings intact, hold
   the 1 button for 5-10 seconds and the lock will automatically turn on.  When the loader
   is started, the lock will always be enabled, so there is no need to manually set the
   lock before you let the kiddies play.  :)
   - NOTE: this option is enabled by default.
 * Added new config option (unlock_password = [BUDAH12]) to allow a custom button
   combination to be used for the admin_lock password.  
   - NOTE: The password length is limited to 10 characters.  Do NOT use quotes around the
     password - just type what you want it to be.  E.g.  unlock_password = 12UDAB
   - The following are the button mappings for the password:
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
 * Automatically hide uLoader's CFG entry, so hide_game=__CF is no longer needed.
 * Added new option on the Game Options screen called "Hide Game".  This allows you
   to set which games are hidden when the admin lock is LOCKED.  In order to see this
   option, admin_lock must be enabled (which it is by default) AND the admin lock must
   be in an unlocked state!
   - NOTE: this functionality completely replaces the hide_game option, but they CAN
     be used together.  Any games currently listed in hide_game will ALWAYS be marked
     as hidden by default in Game Options and cannot be unhidden until they are removed
     from hide_game.  
   - NOTE 2: An easy way to convert all your games in hide_game to this new functionality
     is to start the loader with your hide_game still in config.txt and then go into
     Game Options in any game (you may have to unlock admin lock first) and change
     something and save it.  All your hide_game entries will automatically be saved.  
     Then you can remove the hide_game entry completely from config.txt.
 

06-08-2009 cfg v39c (bugfix)

 * Changed cheats url to: geckocodes.org
 * Fixed SD card access in games
 * GUI page number alignment

06-08-2009 cfg v39 (release)

 * Fixed running games from SDHC
 * GUI Text improvments:
 - Fixed gui_text_color=black
 - gui_text_outline and gui_text_shadow accept black and white too

05-08-2009 cfg v39b (beta)

 * Improved the covers reflection
 * GUI Text improvments:
 - Moved gui_text_outline and gui_text_shadow to theme section
 - Select the outline color automatically if only alpha specified
   (now AA and 000000AA are different)
 - Added gui_text2_* options for text on darkened background in coverflow mode
   (faded background or reflections)
 - Center title in gridflow modes
 - Adjusted title position in gui for overscan
 - Added option: gui_title_top = [0], 1
 - Fixed sometimes unreadable text
 * Added option: gui_lock = [0], 1
   (locks gui style changes with up/down buttons)
 * Increase max title length in titles.txt to 64
 * url tag CC=ZH For W region game id (Taiwan)

01-08-2009 cfg v38 (release)

 * Fixed searching for .mp3 files
 * Fine tuned wiimote pointer scrolling in coverflow
 * Bigger default gui font
 * Added options: gui_text_outline=AA gui_text_shadow=AA
 * Added a better selection of fonts
 * Removed a few old themes (ultimate, stars)
 * Updated titles.txt

20-07-2009 cfg v38b (beta)

 * Added WiiMote pointer scrolling in coverflow modes - as you move the pointer towards 
   the edge of the screen, the covers automatically move.  Also, the movement speed 
   increases as the pointer approaches the edge.
 * Added random music playing - all .mp3 or .mod files in the base
   folder will play randomly (sd:/usb-loader by default)
 * Added new parameter (PATH) to the "music" option in config.txt to allow
   any folder to be specified for music playing (e.g. sd:/mp3s or usb:/mp3s)
 * Pause music while installing new games.
 * Changed option: download_cc_pal = [AUTO], EN, FR, DE, AU, ...
   If AUTO is specified, then the code is set depending on the console
   region - if Australia: AU, Brasil: PO else, console language is used.
   This is now the default, but can still be specified as before.
 * Support for msdos and utf8 formatted ocarina TXT files
 * Always mount USB FAT partition, not just when there is no config.txt on SD.
 * Minor cleanups

25-07-2009 cfg v37 (release)

 * Fixed USB FAT detection on drives larger than 1TB
   (Also FAT detection shold no longer require that the partition is marked active)
 * Fixed covers not showing in favorites mode
 * Minor cleanups

24-07-2009 cfg v37b2 (beta 2)

 * Fixed some issues with the new libfat
 * Display which covers are already present when downloading
   missing covers for a single game.

23-07-2009 cfg v37b (beta)

 * Improved covers loading speed - using new libfat.
   (fixed the issues with usb hdd fat partition in v37ax)
 * Cover downloading improvements: when downloading missing covers,
   it will now check if it's missing for all cover styles not just the current.
   (so that full covers can be easily downloaded, without the need to download everything)
   Also note: pressing RIGHT will download only missing covers, while pressing LEFT will
   force download all covers. This is valid for per-game cover download and for downloading
   covers for all games.
 * Changed WiiMote rotation functionality in coverflow: rotate 90 degrees to flip to the 
   back cover and then rotate back up to 0 degrees to flip to the front.

22-07-2009 cfg v37ax (alpha-experimental ***SEE NOTE!)

 * Same changes as v37a, but with new libfat for improved cover loading speed.
   - NOTE: DO NOT USE THIS if your usb-loader directory is on a USB drive!  It will NOT mount your FAT
     partition.  This is just a preview release for people who have all their covers / themes / 
     config.txt on SD.

22-07-2009 cfg v37a (alpha) (usptactical)

 * Full cover image support in all coverflow gui modes
   - NOTE: Full covers will load in all coverflow modes by default.  If no local full cover is found, 
     the 2D flat cover will be used.
 * Full cover download support - choose <Download All Missing Covers> in Global Options to download
   - NOTE: Full covers go in the "usb-loader/covers/full" directory.
 * Added URL tags for full cover downloads.
 * Twist the WiiMote 90 degrees (right or left) while pointing at the screen to flip the center cover 
   over to display the back.  Up on the D-Pad continues to do the same.
 * Mouseover on cover in coverflow mode highlights cover and shows game title.  Pressing A on the 
   selected cover will load the game, 1 will go to Game Settings, etc.
 * Visual improvements to cover objects in coverflow mode
 
22-07-2009 cfg v36d (bugfix)

 * Fixed error when accessing network and after that using IOS 222/223-mload
 * Improved url tag {CC} region detection for: IT, ES and NL
 * Changed default urls to use {ID6} instead of {ID}

22-07-2009 cfg v36c (bugfix)

 * Fixed url {CC} tag for US region
 * added option: download_cc_pal = [EN], FR, DE, AU, ...
   The code is used as a replacement for {CC} tag for PAL regions.
   If image is not found with the supplied cc code, download is
   retried with EN code.
 * Changed default urls to wiitdb.com site.

20-07-2009 cfg v36 (release)

 * Minor cleanups of the Cheat Manager
   Increased limits for cheat codes (per game):
   max 256 cheats, max 1000 total code lines

 * added {CC} tag for urls (Country Code) based on
   game region id expands to one of: EN FR DE JA KO

19-07-2009 cfg v36b (beta)

 * Ocarina TXT (Cheat Code Manager)
   (based on wiicmpnc and wiiflow)
 * Loadstructor support (launch a game directly)
   (either by binary gameid patch or forwarder param)
 * Online update: size check

18-07-2009 cfg v35d (bugfix)

* Fixed crash after installing a game

15-07-2009 cfg v35c (bugfix)

* Stability improvements for games that require alt.dol.
  Fixes freezes and glitches in MOH2, FIFA08 (maybe others as well)

10-07-2009 cfg v35 (release)

 * Force video mode fixes
 * Online update: progress indicator, fixed crash, compact display
 * Reduced size of embedded graphics
 * Minor cleanups

08-07-2009 cfg v35rc (release candidate)

 * Stability improvements
 * Added all coverflow modes to gui_style option:
   - coverflow3d coverflow2d frontrow vertical carousel
 * Save gui style and rows settings in global options save
 * Online update: improved detection of boot.dol location
 * Fixed garbaged display when printing out a code dump
 * Increase max favorites to 64
 * Updated covers urls

07-07-2009 cfg v35b (beta)

 * Save global settings: theme and device
 * Save selection of alt.dol from disc

06-07-2009 cfg v35a6 (alpha6)

 * Online Update
 * minor fixes

06-07-2009 cfg v35a5 (alpha5)

 * Fixed theme switching
 * Stability fixes
 * Disabled console game list markings.
   Can be re-enabled with options:
   console_mark_page = [0], 1
   console_mark_favorite = [0], 1
   console_mark_saved = [0], 1

05-07-2009 cfg v35a4 (alpha4)

 * Disabled ttf font rendering to speed up cover loading and resolve issue when music
   is playing.
 
04-07-2009 cfg v35a3 (alpha3)

 * Integrated Coverflow Gui mode
   - NOTE: This mode renders the game covers in true 3D so 2D (flat) cover images are needed.
     If "fake" 3D/disk cover images are currently being used (in console or Grid mode), moving to Coverflow 
     will automatically switch to 2D covers and then switch back when leaving.
   - To access Coverflow mode (from console mode) press B and then down several times.
     Each subsequent Down button press will iterate through each coverflow style.
   - Currently only 2D flat front covers are supported.  Full cover image (front, spine,
     back) support will be added in a future release.
 * Favorites in console mode are now marked with a '*'
 * Games with saved options are now marked with a '#' in the last column of the console.

03-07-2009 cfg v35a2 (alpha2)

 * Fixed =+ with cover_url options
 * Changed default urls to this:

cover_url =
cover_url =+ http://wiicover.gateflorida.com/sites/default/files/cover/2D%20Cover/{ID}.png
cover_url =+ http://boxart.rowdyruff.net/flat/{ID}.png
cover_url =+ http://awiibit.com/BoxArt160x224/{ID}.png
cover_url =+ http://wiitdb.com/wiitdb/artwork/cover/EN/{ID}.png
cover_url =+ http://wiitdb.com/wiitdb/artwork/cover/US/{ID}.png

#3d
cover_url_3d =
cover_url_3d =+ http://wiicover.gateflorida.com/sites/default/files/cover/3D%20Cover/{ID}.png
cover_url_3d =+ http://boxart.rowdyruff.net/3d/{ID}.png
cover_url_3d =+ http://awiibit.com/3dBoxArt176x248/{ID}.png
cover_url_3d =+ http://wiitdb.com/wiitdb/artwork/cover3D/EN/{ID}.png
cover_url_3d =+ http://wiitdb.com/wiitdb/artwork/cover3D/US/{ID}.png

#disc
cover_url_disc =
cover_url_disc =+ http://wiicover.gateflorida.com/sites/default/files/cover/Disc%20Cover/{ID}.png
cover_url_disc =+ http://boxart.rowdyruff.net/fulldisc/{ID}.png
cover_url_disc =+ http://awiibit.com/WiiDiscArt/{ID}.png
cover_url_disc =+ http://wiitdb.com/wiitdb/artwork/disc/EN/{ID}.png
cover_url_disc =+ http://wiitdb.com/wiitdb/artwork/disc/US/{ID}.png

 * option: download_all_styles = [1], 0
   Downloading all styles (flat,3d,disc) of covers, or just the current style

02-07-2009 cfg v35a (alpha)

 * Load Alternative .dol from disc (from NeoGamma by WiiPower)

01-07-2009 cfg v34 (release)

 * Changed default URLS to sites:
   1. wiicover.gateflorida.com
   2. boxart.rowdyruff.net
 
 * Retry downloading from all available servers if cover not found
 
 * Multiple URLS can be specified for cover_url_* options
   They can be in the same line separated with space, example:
   cover_url = http://site1.com/{ID}.png http://site2.org/{ID}.png
   Or in multiple lines using =+ to add instead of =, example:
   cover_url = http://site1.com/{ID}.png
   cover_url =+ http://site2.org/{ID}.png
   cover_url =+ http://site3.net/{ID}.png

 * Added another URL tag: {ID} which will try automatically to
   find the correct ID by trying ID6, ID4 and ID3

 * Download all cover styles at the same time: 2d, 3d and disc

 * option: download_id_len = [4], 6
   Specifies the downloaded cover ID length for the saved file name 

 * Print info when loading external ehcmodule

29-06-2009 cfg v34b (beta)

 * Changed the default urls from theotherzone.com to wiiboxart.com
   although, automatic downloading of covers from that site now requires payment,
   which also requires changing of URL to include /USERNAME/PASSWORD/

 * Changed the way cover_url options work:
 - cover_url* are now global instead of theme options.
 - Added per cover style url options:
   cover_url - standard (2d) covers
   cover_url_3d - 3d covers
   cover_url_disc - disc covers
 - Note: These options still work and do the same:
   cover_url_norm, cover_url_3d_norm, cover_url_disc_norm
 - Downloading covers in widescreen mode will no longer download resized
   widescreen covers, instead full covers are downloaded and then resized
   when they are being displayed. In other words, cover_url_*_norm is used
   always instead of cover_url_*_wide in widescreen mode.
   To revert this to previous behaviour, use the option:
   download_wide = [0], 1 Which will use cover_url_*_wide options in widescreen mode:
   cover_url_wide
   cover_url_3d_wide
   cover_url_disc_wide
 - Note: ID_wide.png covers are still used if found in widescreen mode.
   If not found then ID.png is used, same as before.

 * Changed cover_style to not change the cover_url values as was before
   but just selectes the proper url from one of the specified options.

 * Console mode game list improvements:
   - Added '+' indicators if there are more games in up or down directions.
   - Added page indicator
   
 * Use bg_gui_wide.png for widescreen gui background.
   If not found bg_gui.png is used instead.
 
 * Themable gui resources: favorite.png, pointer.png, hourglass.png, font.png
   See inSDRoot/usb-loader/resources for example files:
   - favorite0.png - turns off the favorite star
   - favorite32.png - half sized favorite star
   - favorite64.png - full sized favorite star
   Copy one of the above to sd:/usb-loader/favorite.png to change the favorite star.
   
 * option: start_favorites = [0], 1
   Start with the favorites game list

 * Ocarina codes are now searched in the following directories:
   1. sd:/usb-loader/codes/
   2. sd:/data/gecko/codes/
   3. sd:/codes/

 * Updated ehcmodule to uloader 2.3 (used by ios 222/223-mload)(by Hermes)
   Load external ehcmodule if found in: sd:/usb-loader/ehcmodule.elf

 * Updated Nature theme (by DonTlaloc)


26-06-2009 cfg v33 (release)

 - When switching favorites in gui mode, move to start of the game list

26-06-2009 cfg v33b3 (beta3)

 - Fixed game: Wii Sports Resort
   (by disabling "Sam & Max" fix, which was not working properly anyway)

26-06-2009 cfg v33b2 (beta2)

 - fixed ocarina

26-06-2009 cfg v33b (beta)

 - Favorite Games
   Favorite game is marked in the game options screen. To switch between
   all games and favorite games, press 2 in either gui or console mode.

 - Split game and global options screens
 - More memory for alt.dol - higher loader start address (same as v32t1)
 - Possible alt.dol stability improvement (bss init)
 - If the loader is started from usb drive (fat partition) it will look
   for the configuration first on the usb drive and if not found on sd card.
 - Properly renamed "002 fix" to "Anti 002 fix" in game options screen
 - Fixed crash if ocarina is enabled and MK is started

22-06-2009 cfg v32 (release)

 - New default theme: BlueMatrix (by Narolez)
 - New HBC icon (by Matriculated)

22-06-2009 cfg v32b2 (beta2)

- fix for alt.dol out of memory issues 

22-06-2009 cfg v32b (beta)

 - 002 fix option (002b variant)
 - Updated ios222/223-mload ehci module and dip plugin to uLoader 2.1 version
 - Split VIDTV option from video modes
 - new game compatibility options:
   vidtv = [0], 1
   fix_002 = [0], 1
   block_ios_reload = [0], 1
   alt_dol = [0], 1
 - Remeber saved settings for the above game options

20-06-2009 cfg v32a2 (alpha2)

 - Alternative .dol loading option (from NeoGamma by WiiPower)
   Loaded from loader base dir GAMEID.dol (4 letter ID)
   [ default: sd:/usb-loader/GAMEID.dol ]
 - Sam & Max fix (by WiiPower)
 - Updated ios22[23]-mload ehci module from uLoader 2.0
 - Init wpad after ios reload, so that confirm_ocarina doesn't hang

17-06-2009 cfg v32a1 (alpha1)

 - Block IOS Reload option
   (works only with IOS 222-mload or 223-mload)

02-06-2009 cfg v31c (bug fixes / minor improvements)

 - cIOS 249 rev13 - 002 fix (by WiiPower)
 - Fixed: power off button in GUI mode
 - Fixed: gui style no longer resets when switching to console and back to gui
 - option gui=start will switch to gui directly after device init,
   without first displaying the game list in console mode
 - print IOS version and revision in device selection menu

01-06-2009 cfg v31 (release)

 - option: gui_style = [grid], flow, flow-z
   Set the default GUI style
 - option: gui_rows = [2] (Valid values: 1-4)
   Set the default number of rows in GUI mode
 - gui_text_color = [black], white, HEX
   Set the text color in GUI mode
   Note: to use a color other than a black or white, it has to be
   represented as a HEX value with the following components: RRGGBBAA
   RR=red, GG=green, BB=blue, AA=alpha
   Example: red = FF0000FF, blue = 00FF00FF, yellow = 00FFFFFF
 - Return directly back to GUI mode if any action is started from GUI mode
   Also return to GUI if the action is canceled.
   Action refers to one of: install, remove, options, run game
 - Re-enabled buttons=options_B
   If options_B is used then GUI mode switching is done with button 1
 - Changed option covers_path to global instead of theme option
   In addition the following options are added:
   option: covers_path_2d
   option: covers_path_3d
   option: covers_path_disc
   cover_style will then select which of the above paths is used
   Changing covers_path will change all covers_path_* like this: 
   covers_path_2d = covers_path
   covers_path_3d = covers_path/3d
   covers_path_disc = covers_path/disc
   If you need fine controll on the 2d/3d/disc paths use the
   covers_path_* after covers_path.
 - Per-game save settings: country_patch, ios


31-05-2009 cfg v31b2 (beta2)

 - GUI: New style: "flow-z"
 - Country Patch for better compatibility with some games (by WiiPower)
   (for Punch Out & EA Sports on JPN consoles, ...)
   Settings menu option: "Country Patch"
   config option: country_patch = [0], 1
 - Fixes to IOS 222-mload/223-mload
   (fixed occasional hang at start when ios was set in config.txt)
 - Hide uLoader's CFG entry (hide_game=__CF) in default config.txt
 - Updated titles.txt
 - Updated Wolf_3D theme gui background

28-05-2009 cfg v31b: (beta)

 - GUI: New style: "grid flow"
   To change gui style mode use button up when in 4 rows mode
   or button down when in 1 row mode
 - GUI: Animated transition between rows change and style change
 - Fixes to IOS 222-mload/223-mload
 - If custom IOS is specified, load it before storage device init
 - Added game loading progress indication when using custom IOS

25-05-2009 cfg v31a: (alpha)

 - Support USB drive with a FAT partition for configuration and covers
   (If config.txt is not found on SD, then uses USB FAT partition)
 - Support for kwiirk and hermes cIOS 222
   option: ios = [249], 222-yal, 222-mload, 223-yal, 223-mload, 250
   Note: 222-yal is for kwiirk's cIOS
   Note: 222-mload is for Hermes's cIOS
   Note: a few seconds of delay when starting a game with custom ios is expected.
 - Added IOS selection to options screen
 - GUI Mode screenshot enabled with option: home = screenshot
 - GUI: added fade transition effect from console
 - option: gui_transition = [scroll], fade
   Set GUI transition effect between console and gui mode
 - GUI: animated transition when changing number of rows
 - minor GUI fixes


20-05-2009 cfg v30: (release)

 - Fixed crash when switching theme after gui mode
 - Other minor GUI fixes

20-05-2009 cfg v30b: (beta)

 - GUI Mode (beta)
   By default console mode is started.
   To switch to GUI mode, press B in main screen.
   While in GUI mode, the following buttons are used:
     button A : start selected game
     button B : return to console mode
     button LEFT/RIGHT : previous/next page
     button UP/DOWN : switch number of rows (1-3)
     button 1, +, - : options, install, remove
     button HOME : exit
   The background image in GUI mode can be changed with the file:
     sd:/usb-loader/themes/Theme_Name/bg_gui.png  or
     sd:/usb-loader/bg_gui.png
 - option: gui = [1], 0, start
   Enable or disable GUI mode.
   Using gui = start will start directly in GUI mode when loader is started.
 - Start music only after usb hdd device is opened, to avoid stuttering
   while initializing the usb device.
 - Other minor fixes


19-05-2009 cfg v30x: (experimental)

 - GUI mode (experimental)
 - fixed crash when using some forwarders 

15-05-2009 cfg v29d:

 - Fixed option: hide_game=...

12-05-2009 cfg v29c:

 - Fixed minor glitch in 576i mode

12-05-2009 cfg v29:

 - Theme support (See README-CFG.txt for details on how to create themes)
 - option: theme = Theme_Name
   Load a specified theme from sd:/usb-loader/themes/Theme_Name/theme.txt
 - Runtime theme change from the options menu
 - Fixed mp3 stuttering
 - Now mp3 plays fine also while installing a new game.
 - Faster loading of game covers and scrolling through the game list
 - Now it works fast also with large (1000+) collection of covers in same directory
 - Fixed: searching for music.mp3 if base directory is not sd:/usb-loader
 - removed obsolete option: buttons=ultimate

05-05-2009 cfg v28:

 - Screenshot is saved to screenshot[1..99].png - the first which doesn't exist.

 - More flexible base path - it will search for config.txt in:
   sd:/usb-loader/, sd:/USBLoader/ and app dir which by default
   is: sd:/apps/USBLoader, but can be something else if started with
   homebrew channel from a different location, for example: sd:/apps/my_usb_loader/
   The location where config.txt is first found is then used as a base for all other
   files such as: titles.txt, settings.cfg, music, backgrounds, covers and screenshots.
   Note1: the config.txt and titles.txt in appdir will be read in addition even
   if the base path is one of the global paths like sd:/usb-loader.
   Note2: background and covers paths can still be set to any path
   using appropriate config options.

 - option: cursor = ">>"
   Changes cursor shape, at max 2 characters are used. Can be empty.
   If you want spaces, so that the menu is not shifted use quotes
   like this: cursor = "  "

 - option: menu_plus  = "[+] "
   Changes the [+] mark in the main, options and start menu.
   At max 4 characters are used.


03-05-2009 cfg v27:

 - Clear button status before formatting, so it always asks for confirmation
 - Allow to exit from device menu with button B, if device has already been
   selected previously
 - Fixed crash on device retry-on-failure timeout (no ios reload)
 - option: ios = [249], 222

02-05-2009 cfg v26:

 - Fixed PNG transparency
 - Clear button status after install, so it always asks for confimration

01-05-2009 cfg v25:

 - Automatic resize of covers (4:3 -> wide) if wide cover not found.
 It can actually resize any size to specified size with options:
 covers_size = width, height
   default: covers_size = 160, 225
 wcovers_size = width, height
   default: wcovers_size = 130, 225
   used for widescreen setting. If not set it will use the covers_size
   and properly scaling it to widescreen size.
 With these changes in place the loader is now compatible with
 the 3d cover pack by NeoRame:
 http://rs777.rapidshare.com/files/227304261/3D_Cover_Update_29_April_2009.rar
 - The supplied noimage.png images in covers/3d and covers/disc
 now use transparency (tnx narolez)

30-04-2009 cfg v24:

 - Support transparent PNG for covers (used by 3d and disc covers)
 - Changed builtin background and nocover images to match the default provided inSDroot.
 - Changed default layout to large3 (to match builtin background)

29-04-2009 cfg v23:

 - Game compatibility fixes:
 - Fixed harvest moon & nunchuck problem
 - Added video mode: VIDTV
   (It was previously enabled by default, but is not compatible with all games
    so it is now a separate mode. Select it in game options / video mode.
       Required for Japanese and maybe some other games)
 - Fixed network problems in games when downloading covers from loader.
 - Restart music if it was stopped by usb device retry on failure

27-04-2009 cfg v22:

 - option: console_transparent = [0], 1
   Enable transparent console.
 - Auto repeat Up and Down movement if button is held
 - Allow re-download of a cover if it exists
 - Added layout=kosaic


27-04-2009 cfg v21:

 - Fixed downloading of NTSC covers.
 - Check if downloaded file is a valid PNG
 - Check for a valid HTTP status of downloaded file
 - New set of noimage.png for 3d and disc covers (tnx Narolez)
 - Clear console on init (Narolez)
 - option: home = screenshot
   make a screenshot when home button is pressed (only in main and options screens)
 - option: confirm_ocarina = [0], 1
 - option: cursor_jump = [0] or N
   Sets how much moves left/right (if 0 do a end page / next page jump)


25-04-2009 cfg v20:

 - Added option to 'Download All Missing Covers' in the options menu.
   If selected with dpad-right, only missing covers will be donwloaded
   If selected with dpad-left, ALL covers of installed games will be downloaded

 - Support for 3d covers and disc covers
   option: cover_style = [standard], 3d, disc
   This option also changes the cover_url, covers_path and cover_size
   covers_path will be set to:
   standard: sd:/usb-loader/covers
   3d:       sd:/usb-loader/covers/3d
   disc:     sd:/usb-loader/covers/disc

 - New set of backgrounds suitable for 3d covers

 - Custom urls.
   URL can contain any of the following tags which are then replaced
   with proper values: {REGION}, {WIDTH}, {HEIGHT}, {ID6}, {ID4}, {ID3}

   option: cover_url_norm = URL
   (url for normal 4:3 covers)
   default:
      cover_url_norm = http://www.theotherzone.com/wii/{REGION}/{ID6}.png

   option: cover_url_wide = URL
   (url for widescreen covers)
   default:
      cover_url_wide = http://www.theotherzone.com/wii/widescreen/{REGION}/{ID6}.png

   option: cover_url = URL
   (This changes the url for both normal and widescreen covers)

 - Debugging can be enabled with option: debug=1
   It will also show some music related info


24-04-2009 cfg v19:

 - Fixed music stuttering
 - Fixed mp3 loop crash

23-04-2009 cfg v18:

 - Improved USB and SD retry initialize on fail (tnx: Narolez)
 - Improved mp3 loading
 - Added support for mod files as background music
 - option: music = [1], 0, filename
   option music will now accept a filename (.mp3 or .mod) which
   can be relative to sd:/usb-loader or an absolute pathname
   if option is set to: music = 1 then it will search for music.mp3
   or music.mod whichever is found first in sd:/usb-loader

22-04-2009 cfg v17:

 - Eject DVD after install is complete if button A is pressed
 - option: hide_game = [0], GAMEID1, GAMEID2, ...
   Hide games from list (can be used for parental control)
   Multiple games can be specified in one line separated by comma ","
   or each game in a separate hide_game = GAMEID line.
   setting hide_game = 0 will reset the hide list.
   GAMEID is a 4 letter game ID.
   Example: hide_game = RZZP, RDCP
 - option: pref_game = [0], GAMEID1, GAMEID2, ...
   Preffered games, to be shown first in the list.
   Syntax is same as with hide_game.
   Example: pref_game = RHAP, RSSP
 - Show number of games after hdd info

21-04-2009 cfg v16:

 - fixed music sometimes not working
 - widescreen support
   option: widescreen=[auto], 0, 1
   If widescreen is enabled (or autodetected) then the following options are used:
   option: wbackground=filename.png
   option: wconsole_coords=x,y,w,h
   option: wcovers_coords=x,y
   Note 1: widescreen will be enabled only if the file specified by wbackground
   is found, otherwise it will fall back to normal mode.
   Note 2: cover images have to be named GAMEID_wide.png
   downloading covers will save them with this name automatically.
   Note 3: some layouts will specify widescreen cooridinates automatically
   like: large3 and ultimate3, so there is no need to specify them manually,
   if one of these layouts are used.

20-04-2009 cfg v15:

 - Added Force PAL50, PAL60, NTSC video modes (tnx Narolez)
   option: video=pal50, pal60, ntsc
 - Light up DVD slot when install finished (Dteyn/Bool)
 - BETA: mp3 background music - plays a sd:/usb-loader/music.mp3 if found.
   NOTE: sometimes the audio stutters, not sure how to fix it
   Can be disabled with option: music=0


19-04-2009 cfg v14:

 - Colored console text:
   option: colors=[dark], light, mono
   will select a predefined set of colors for a dark or light background
   or normal 2 color text if mono is specified
   Individual text colors can be specified with options:   
   color_header, color_selected_fg, color_selected_bg,
   color_inactive, color_footer, color_help
 - Fixed individual disable_xxx settings. (tnx. Narolez)

18-04-2009 cfg v13:

 - Support running games from a SD/SDHC Card with a WBFS and FAT32 partition.
   So you can have the loader, covers, background and other configuration files
   on the FAT partition and games on WBFS partition.
   Note: this worked in waninkoko original 1.4 but seems broken in 1.5.
 - Minor fixes to download covers from internet (tnx: Don Killah)
 - Fixed simple=1 option

18-04-2009 cfg v12:

 - fine granularity of simple options:
   added options: disable_remove, disable_install, disable_options, disable_format
   by default none of this is set.
   setting simple=1 will change all of these disable_xxx options and
   hide_hddinfo and hide_footer to 1 and confirm_start to 0.
   setting simple=0 will do the opposite.
   any of the disable_xxx, hide_xxx and confirm_start options can be set individually.
 - allow absolute paths for background (like sd:/somedir/myimage.png)
   if the specified background is not an absolute path it is searched
   in default directory (sd:/usb-loader)
 - option: install_partitions = [all], only_game
 - minor stuff: background version, fixed default apps/USBLoader/config.txt



17-04-2009 cfg v11:

 - Rebase code to waninkoko SDUSB Loader 1.5

16-04-2009 cfg v10:

 - load game covers from the options menu (by hungyip84/forsaken)
 - fixed Japan (and other regions) games (tnx: Narolez)
 - support for covers with 4 letter ID like: RHAP.png
   (6 letter ID file names like: RHAP01.png are of course still supported.)
   Covers that are downloaded from the options menu will be automatically saved
   to a file with 4 letter ID instead of 6.

16-04-2009 cfg v9:

 - new set of backgrounds to match the new buttons=options mode
 - option: layout=large3 to match the new backgrounds
 - automatically set number of entries per page
   and number of characters from the console size
 - buttons=options_B for alternative button layout (press B to enter options menu)
 - option: layout=ultimate3 to match WiiShiza backgrounds from ultimate V 3-7
   note: ultimate 7 background use in combination with buttons=options_B
 - options to control the look of main menu:
   option: hide_header = [0],1 
   option: hide_hddinfo = [0],1 
   option: hide_hfooter = [0],1 
 - included titles.txt from: http://wiki.gbatemp.net/wiki/index.php/USB_Loader_titles.txt
 - cleaned up the sample config files to match the sample backgrounds
 - remove confirmation for Ocarina, since it is configurable
 - fixed display of titles longer than console
 - fixed a crash with odd x coordinates
 - other minor fixes

15-04-2009 cfg v8:

 - option: device=[ask], usb, sdhc
 - option: confirm_start=[1], 0
 - option: buttons=[options], original
 With buttons=options mode, you can edit options in a menu:
 direction buttons - select and change options
 button 1 - enter options menu
 button 2 - save/discard game options

14-04-2009 cfg v7:

 - Added per-game saved settings (video, language, ocarina)
   Per-game settings can be saved/forgotten in the start game
   screen with button 2.
   The settings are saved to sd:/usb-loader/settings.cfg

13-04-2009 cfg v6:

 - option: buttons=[original], ultimate  (change button controls layout.)
   The button layout "ultimate" is:
       BUTTON 1 - force video option
       BUTTON 2 - ocarina option
       BUTTON B - language option

13-04-2009 cfg v5:

 - merge changes from hungyip84 Ultimate V6:
 - language selection (config option: language=...)
 - video modes (config option: video=system, game, patch)

13-04-2009 cfg v4:

 - Rebase code to waninkoko SDUSB Loader 1.4

12-04-2009 cfg v3:

 - Rebase code to waninkoko SDUSB Loader 1.3
 - standard SD mode for background and cover access still works
   (waninkoko's requires new cios with SDHC support)
 - changed default base path from sd:/USBLoader to sd:/usb-loader
   so it uses the same path as waninkoko.
   (If new path is not found, it reverts to the old one)
 - fix ocarina path
 - fix loading config from HBC app dir (/app/usbloader_cfg/config.txt)
 - option: console_entries=N (number of shown games in list)
 - option: layout=original2 (waninkoko 1.2-1.3)

11-04-2009 cfg v2:

 - option: layout=large2 matches usptactical cover coordinates
 - option: layout=ultimate1 (WiiShizza)
 - option: layout=ultimate2 (jservs7 / hungyip84)
 - option: console_coords=x,y,width,height
 - option: console_color=foreground,background (color values: 0-15)
 - option: covers_coords=x,y
 - option: covers_path=path
 - show noimage.png if cover missing
 - sd bug fix (56Killer)

11-04-2009 cfg v1:

Based on Wanikoko & Kwiirk USB Loader 1.1 + nIxx mod (USBLoader1.1ssorgmod+cover)
including:
 - Sorg mod1.02 (Sorg)
 - Ocarina (fishears)
 - Cover (usptactical)
 - Video Force
 - Simple / childproof
 - Config, Title rename (oggzee)

