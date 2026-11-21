/*____________________________________________________________________
|
| File: main.cpp
|
| Description: Main module in program.
|
| Functions:  Program_Get_User_Preferences
|             Program_Init
|							 Init_Graphics
|								Set_Mouse_Cursor
|             Program_Run
|							 Init_Render_State
|             Program_Free
|             Program_Immediate_Key_Handler               
|
| (C) Copyright 2013 Abonvita Software LLC.
| Licensed under the GX Toolkit License, Version 1.0.
|___________________________________________________________________*/

#define _MAIN_

/*___________________
|
| Include Files
|__________________*/

#include <first_header.h>
#include "dp.h"
#include "..\Framework\win_support.h"
#include <rom8x8.h>

#include "main.h"
#include "position.h"

/*___________________
|
| Type definitions
|__________________*/

typedef struct {
  unsigned resolution;
  unsigned bitdepth;
} UserPreferences;

/*___________________
|
| Function Prototypes
|__________________*/

static int Init_Graphics (unsigned resolution, unsigned bitdepth, unsigned stencildepth, int *generate_keypress_events);
static void Set_Mouse_Cursor ();
static void Init_Render_State ();

/*___________________
|
| Constants
|__________________*/

#define MAX_VRAM_PAGES  2
#define GRAPHICS_RESOLUTION  \
  (                          \
    gxRESOLUTION_640x480   | \
    gxRESOLUTION_800x600   | \
    gxRESOLUTION_1024x768  | \
    gxRESOLUTION_1152x864  | \
    gxRESOLUTION_1280x960  | \
    gxRESOLUTION_1400x1050 | \
    gxRESOLUTION_1440x1080 | \
    gxRESOLUTION_1600x1200 | \
    gxRESOLUTION_1152x720  | \
    gxRESOLUTION_1280x800  | \
    gxRESOLUTION_1440x900  | \
    gxRESOLUTION_1680x1050 | \
    gxRESOLUTION_1920x1200 | \
    gxRESOLUTION_2048x1280 | \
    gxRESOLUTION_1280x720  | \
    gxRESOLUTION_1600x900  | \
    gxRESOLUTION_1920x1080   \
  )
#define GRAPHICS_STENCILDEPTH 0
#define GRAPHICS_BITDEPTH (gxBITDEPTH_24 | gxBITDEPTH_32)

#define SCREENSHOT_FILENAME "screenshots\\screen"

#define AUTO_TRACKING    1
#define NO_AUTO_TRACKING 0

/*____________________________________________________________________
|
| Function: Program_Get_User_Preferences
|
| Input: Called from CMainFrame::Init
| Output: Allows program to popup dialog boxes, etc. to get any user
|   preferences such as screen resolution.  Returns preferences via a
|   pointer.  Returns true on success, else false to quit the program.
|___________________________________________________________________*/

int Program_Get_User_Preferences (void **preferences)
{
    static UserPreferences user_preferences;

    if (gxGetUserFormat (GRAPHICS_DRIVER, GRAPHICS_RESOLUTION, GRAPHICS_BITDEPTH, &user_preferences.resolution, &user_preferences.bitdepth)) {
          *preferences = (void *)&user_preferences;
          return (1);
    }
    else
      return (0);
}

/*____________________________________________________________________
|
| Function: Program_Init
|
| Input: Called from CMainFrame::Start_Program_Thread()
| Output: Starts graphics mode.  Returns # of user pages available if 
|       successful, else 0.
|___________________________________________________________________*/
 
int Program_Init (void *preferences, int *generate_keypress_events)
{
    UserPreferences *user_preferences = (UserPreferences *) preferences;
    int initialized = FALSE;

    if (user_preferences) 
      initialized = Init_Graphics (user_preferences->resolution, user_preferences->bitdepth, GRAPHICS_STENCILDEPTH, generate_keypress_events);
      
    return (initialized);
}

/*____________________________________________________________________
|
| Function: Init_Graphics
|
| Input: Called from Program_Init()
| Output: Starts graphics mode.  Returns # of user pages available if 
|       successful, else 0.
|___________________________________________________________________*/

static int Init_Graphics (unsigned resolution, unsigned bitdepth, unsigned stencildepth, int *generate_keypress_events)
{
    int num_pages;
    byte *font_data;
    unsigned font_size;

/*____________________________________________________________________
|
| Init globals
|___________________________________________________________________*/

  Pgm_num_pages   = 0;
  Pgm_system_font = NULL;

/*____________________________________________________________________
|
| Start graphics mode and event processing
|___________________________________________________________________*/

  font_data = font_data_rom8x8;
  font_size = sizeof(font_data_rom8x8);
                                                                      
  // Start graphics mode                                      
  num_pages = gxStartGraphics (resolution, bitdepth, stencildepth, MAX_VRAM_PAGES, GRAPHICS_DRIVER);
  if (num_pages == MAX_VRAM_PAGES) {
    // Init system, drawing fonts 
    Pgm_system_font  = gxLoadFontData (gxFONT_TYPE_GX, font_data, font_size);
    // Make system font the default drawing font 
    gxSetFont (Pgm_system_font);

    // Start event processing
    evStartEvents (evTYPE_MOUSE_LEFT_PRESS     | evTYPE_MOUSE_RIGHT_PRESS   |
                   evTYPE_MOUSE_LEFT_RELEASE   | evTYPE_MOUSE_RIGHT_RELEASE |
                   evTYPE_MOUSE_WHEEL_BACKWARD | evTYPE_MOUSE_WHEEL_FORWARD |
                   // evTYPE_KEY_PRESS | 
                   evTYPE_RAW_KEY_PRESS | evTYPE_RAW_KEY_RELEASE,
                   AUTO_TRACKING, EVENT_DRIVER);                              
                *generate_keypress_events = FALSE;  // true if using evTYPE_KEY_PRESS in the above mask

	      	// Set a custom mouse cursor
	      	Set_Mouse_Cursor ();

        // Set globals
        Pgm_num_pages = num_pages;
    }

    return (Pgm_num_pages);
}

/*____________________________________________________________________
|
| Function: Set_Mouse_Cursor
|
| Input: Called from Init_Graphics()
| Output: Sets default mouse cursor.
|___________________________________________________________________*/
     
static void Set_Mouse_Cursor ()
{
    gxColor fc, bc;

    // Set cursor to a medium sized red arrow
    fc.r = 255;
    fc.g = 0;
    fc.b = 0;
    fc.a = 0;
    bc.r = 1;
    bc.g = 1;
    bc.b = 1;
    bc.a = 0;
    msSetCursor (msCURSOR_MEDIUM_ARROW, fc, bc);
}

/*____________________________________________________________________
|
| Function: Program_Run
|
| Input: Called from Program_Thread()
| Output: Runs program in the current video mode.  Begins with mouse
|   hidden.
|___________________________________________________________________*/
                    
void Program_Run ()
{
    int quit;
    evEvent event;
    gx3dDriverInfo dinfo;
    gxColor color;
    char str[256];

    // Game Timer Variables
    int gameTimeLimit = 60000; // 60 seconds in milliseconds
    int timeRemaining = 0;
    int finalScore = 0; // To "lock" the score when the game ends

    // Game States
    enum GameState { STATE_TITLE, STATE_PLAYING, STATE_HELP, STATE_CREDITS, STATE_GAMEOVER };
    GameState g_CurrentState = STATE_TITLE;

    int score = 0;

    int render, ctrl_pressed, alt_pressed, take_screenshot;

	gx3dObject *obj_tree, *obj_skydome, *obj_ground, *obj_duck, *obj_ouch, *obj_halo1, *menuButton, *crosshair;
	gx3dMatrix m, m1, m2, m3, m4, m5;
    gx3dColor color3d_white    = { 1, 1, 1, 0 };
    gx3dColor color3d_dim      = { 0.1f, 0.1f, 0.1f };
    gx3dColor color3d_black    = { 0, 0, 0, 0 };
    gx3dColor color3d_darkgray = { 0.3f, 0.3f, 0.3f, 0 };
    gx3dColor color3d_gray     = { 0.5f, 0.5f, 0.5f, 0 };
    gx3dMaterialData material_default = {
      { 1, 1, 1, 1 }, // ambient color
      { 1, 1, 1, 1 }, // diffuse color
      { 1, 1, 1, 1 }, // specular color
      { 0, 0, 0, 0 }, // emissive color
      10              // specular sharpness (0=disabled, 0.01=sharp, 10=diffused)
    };

    // --- Duck Hunt Variables ---
    #define NUM_DUCKS 5
    gx3dVector duckPosition[NUM_DUCKS];
    float duckSpeed[NUM_DUCKS];
    bool duckActive[NUM_DUCKS];
    gx3dSphere duckSphere[NUM_DUCKS];
    bool duckOnScreen[NUM_DUCKS];

    // Square Spawn Boundaries
    const float X_MIN = -40.0f;
    const float X_MAX = 40.0f;
    const float Z_MIN = 10.0f;
    const float Z_MAX = 80.0f;
    const float FLY_CEILING = 70.0f; // Height at which they respawn

    // // How to use C++ print outupt 
    // string mystr;

    //mystr = "SUPER!";
    //char ss[256];
    //strcpy (ss, mystr.c_str());
    //debug_WriteFile (ss);

/*____________________________________________________________________
|
| Print info about graphics driver to debug file.
|___________________________________________________________________*/

    gx3d_GetDriverInfo (&dinfo);
    debug_WriteFile ("_______________ Device Info ______________");
    sprintf (str, "max texture size: %dx%d", dinfo.max_texture_dx, dinfo.max_texture_dy);
    debug_WriteFile (str);
    sprintf (str, "max active lights: %d", dinfo.max_active_lights);
    debug_WriteFile (str);
    sprintf (str, "max user clip planes: %d", dinfo.max_user_clip_planes);
    debug_WriteFile (str);
    sprintf (str, "max simultaneous texture stages: %d", dinfo.max_simultaneous_texture_stages);
    debug_WriteFile (str);
    sprintf (str, "max texture stages: %d", dinfo.max_texture_stages);
    debug_WriteFile (str);
    sprintf (str, "max texture repeat: %d", dinfo.max_texture_repeat);
    debug_WriteFile (str);
    debug_WriteFile ("__________________________________________");

/*____________________________________________________________________
|
| Initialize the graphics state
|___________________________________________________________________*/

    // Set 2d graphics state
    Pgm_screen.xleft   = 0;
    Pgm_screen.ytop    = 0;
    Pgm_screen.xright  = gxGetScreenWidth() - 1;
    Pgm_screen.ybottom = gxGetScreenHeight() - 1;
    gxSetWindow (&Pgm_screen);
    gxSetClip   (&Pgm_screen);
    gxSetClipping (FALSE);

	// Set the 3D viewport
	gx3d_SetViewport (&Pgm_screen);
	// Init other 3D stuff
	Init_Render_State ();

/*____________________________________________________________________
|
| Init support routines
|___________________________________________________________________*/

    gx3dVector heading, position;
    
    // Set starting camera position
    position.x = 0;
    position.y = 5;
    position.z = -100;
    // Set starting camera view direction (heading)
    heading.x = 0;  // {0,0,1} for cubic environment mapping to work correctly
    heading.y = 0;
    heading.z = 1;
    Position_Init (&position, &heading, RUN_SPEED);

/*____________________________________________________________________
|
| Init 3D graphics
|___________________________________________________________________*/

    // Set projection matrix
    float fov = 60; // degrees field of view
    float near_plane = 0.1f;
    float far_plane = 1000;
    gx3d_SetProjectionMatrix (fov, near_plane, far_plane);
    
    gx3d_SetFillMode (gx3d_FILL_MODE_GOURAUD_SHADED);
    
    // Clear the 3D viewport to all black
    color.r = 0;
    color.g = 0;
    color.b = 0;
    color.a = 0;

/*____________________________________________________________________
|
| Load 3D models
|___________________________________________________________________*/

	// Load a 3D model																								
    gx3d_ReadLWO2File("Objects\\skydome.lwo",&obj_skydome,gx3d_VERTEXFORMAT_DEFAULT,gx3d_DONT_LOAD_TEXTURES);
    gx3d_ReadLWO2File("Objects\\ground.lwo",&obj_ground,gx3d_VERTEXFORMAT_DEFAULT,gx3d_DONT_LOAD_TEXTURES);
    gx3d_ReadLWO2File("Objects\\duck.lwo",&obj_duck,gx3d_VERTEXFORMAT_DEFAULT,gx3d_DONT_LOAD_TEXTURES);
    gx3d_ReadLWO2File("Objects\\ouch.lwo",&obj_ouch,gx3d_VERTEXFORMAT_DEFAULT,gx3d_DONT_LOAD_TEXTURES);
    gx3d_ReadLWO2File("Objects\\menuButton.lwo", &menuButton, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);
    gx3d_ReadLWO2File("Objects\\crosshair.lwo", &crosshair, gx3d_VERTEXFORMAT_DEFAULT, gx3d_DONT_LOAD_TEXTURES);

    gx3dTexture tex_skydome   = gx3d_InitTexture_File("Objects\\Images\\bright_sky_d128.bmp",0,0);
    gx3dTexture tex_ground    = gx3d_InitTexture_File("Objects\\Images\\sand_d512.bmp",0,0);
    gx3dTexture tex_duck      = gx3d_InitTexture_File("Objects\\Images\\duck.bmp","Objects\\Images\\duck_fa.bmp",0);
    gx3dTexture tex_ouch      = gx3d_InitTexture_File("Objects\\Images\\Numbers\\plusOnePoint.bmp","Objects\\Images\\Numbers\\plusOnePoint_fa.bmp",0);
    gx3dTexture tex_crosshair = gx3d_InitTexture_File("Objects\\Images\\crosshair.bmp", "Objects\\Images\\crosshair_fa.bmp", 0);

    // On-Screen Text Textures
    gx3dTexture tex_title, tex_start, tex_help, tex_helpInfo, tex_creditsButton, tex_credits, tex_gameover, tex_quit;

    tex_title = gx3d_InitTexture_File("Objects\\Images\\text\\title.bmp", "Objects\\Images\\text\\title_fa.bmp", 0);
    tex_start = gx3d_InitTexture_File("Objects\\Images\\text\\start.bmp", "Objects\\Images\\text\\start_fa.bmp", 0);
    tex_help = gx3d_InitTexture_File("Objects\\Images\\text\\help.bmp", "Objects\\Images\\text\\help_fa.bmp", 0);
    tex_helpInfo = gx3d_InitTexture_File("Objects\\Images\\text\\helpInfo.bmp", "Objects\\Images\\text\\helpInfo_fa.bmp", 0);
    tex_creditsButton = gx3d_InitTexture_File("Objects\\Images\\text\\creditsButton.bmp", "Objects\\Images\\text\\creditsButton_fa.bmp", 0);
    tex_credits = gx3d_InitTexture_File("Objects\\Images\\text\\credits.bmp", "Objects\\Images\\text\\credits_fa.bmp", 0);
    tex_gameover = gx3d_InitTexture_File("Objects\\Images\\text\\gameOver.bmp", "Objects\\Images\\text\\gameOver_fa.bmp", 0);
    tex_quit = gx3d_InitTexture_File("Objects\\Images\\text\\quit.bmp", "Objects\\Images\\text\\quit_fa.bmp", 0);

    // On-screen Numbers Textures

    gx3dTexture n0, n1, n2, n3, n4, n5, n6, n7, n8, n9;

    n0 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n0.bmp", "Objects\\Images\\Numbers\\n0_fa.bmp", 0);
    n1 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n1.bmp", "Objects\\Images\\Numbers\\n1_fa.bmp", 0);
    n2 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n2.bmp", "Objects\\Images\\Numbers\\n2_fa.bmp", 0);
    n3 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n3.bmp", "Objects\\Images\\Numbers\\n3_fa.bmp", 0);
    n4 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n4.bmp", "Objects\\Images\\Numbers\\n4_fa.bmp", 0);
    n5 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n5.bmp", "Objects\\Images\\Numbers\\n5_fa.bmp", 0);
    n6 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n6.bmp", "Objects\\Images\\Numbers\\n6_fa.bmp", 0);
    n7 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n7.bmp", "Objects\\Images\\Numbers\\n7_fa.bmp", 0);
    n8 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n8.bmp", "Objects\\Images\\Numbers\\n8_fa.bmp", 0);
    n9 = gx3d_InitTexture_File("Objects\\Images\\Numbers\\n9.bmp", "Objects\\Images\\Numbers\\n9_fa.bmp", 0);

    int numDucksHit = 0;

    const int MAX_OUCH = 20;
    gx3dVector ouchPosition[MAX_OUCH];
    int ouchTimer[MAX_OUCH];   // in milliseconds
    int ouchIndex = 0;         // index into ouch array to create next 'ouch'

/*____________________________________________________________________
|
| Initialize the sound library
|___________________________________________________________________*/

    snd_Init(44, 16, 2, 0, 1);

    Sound s_soundtrack, s_footstep, s_ouch;

    s_soundtrack = snd_LoadSound("Sounds\\mapleLeafRag.wav", snd_CONTROL_VOLUME, 0);
    s_footstep = snd_LoadSound("Sounds\\footstep01.wav", snd_CONTROL_VOLUME, 0);

/*____________________________________________________________________
|
| create lights
|___________________________________________________________________*/

	gx3dLight dir_light;
	gx3dLightData light_data;
	light_data.light_type = gx3d_LIGHT_TYPE_DIRECTION;
	light_data.direction.diffuse_color.r = 1;
	light_data.direction.diffuse_color.g = 1;
	light_data.direction.diffuse_color.b = 1;
	light_data.direction.diffuse_color.a = 0;
	light_data.direction.specular_color.r = 1;
	light_data.direction.specular_color.g = 1;
	light_data.direction.specular_color.b = 1;
	light_data.direction.specular_color.a = 0;
	light_data.direction.ambient_color.r = 0;
	light_data.direction.ambient_color.g = 0;
	light_data.direction.ambient_color.b = 0;
	light_data.direction.ambient_color.a = 0;
	light_data.direction.dst.x = -1;
	light_data.direction.dst.y = -1;
	light_data.direction.dst.z = 0;
	dir_light = gx3d_InitLight (&light_data);

	gx3dLight point_light1;
	light_data.light_type =	gx3d_LIGHT_TYPE_POINT;
	light_data.point.diffuse_color.r = 1;  // red light
	light_data.point.diffuse_color.g = 1;
	light_data.point.diffuse_color.b = 1;
	light_data.point.diffuse_color.a = 0;
	light_data.point.specular_color.r = 1;
	light_data.point.specular_color.g = 1;
	light_data.point.specular_color.b = 1;
	light_data.point.specular_color.a = 0;
	light_data.point.ambient_color.r = 0;  // ambient turned offf
	light_data.point.ambient_color.g = 0;
	light_data.point.ambient_color.b = 0;
	light_data.point.ambient_color.a = 0;
	light_data.point.src.x = 10;
	light_data.point.src.y = 20;
	light_data.point.src.z = 0;
	light_data.point.range = 200;
	light_data.point.constant_attenuation = 0;
	light_data.point.linear_attenuation = 0.1;
	light_data.point.quadratic_attenuation = 0;
	point_light1 = gx3d_InitLight (&light_data);

	gx3dVector light_position = { 10, 20, 0 }, xlight_position;
	float angle = 0;

/*____________________________________________________________________
|
| Flush input queue
|___________________________________________________________________*/
    
	int move_x, move_y;	// mouse movement counters

	// Flush input queue
    evFlushEvents ();
	// Zero mouse movement counters
    msGetMouseMovement (&move_x, &move_y);  // call this here so the next call will get movement that has occurred since it was called here                                    
    // Hide mouse cursor
    msHideMouse ();
	
/*____________________________________________________________________
|
| Main game loop
|___________________________________________________________________*/

	// Variables
    unsigned elapsed_time, last_time, new_time;
	bool force_update;
	unsigned cmd_move;

	// Init loop variables
	cmd_move = 0;
	last_time = 0;
	force_update = false;

    bool draw_wireframe = false;

	// Game loop
	for (quit=FALSE; NOT quit; ) {

		angle += 0.5;
		if (angle >= 360)
			angle = 0;
		gx3d_GetRotateYMatrix (&m, angle);
		gx3d_MultiplyVectorMatrix (&light_position, &m, &(light_data.point.src));
        gx3d_UpdateLight (point_light1, &light_data);

        render = TRUE;
        take_screenshot = FALSE;

/*____________________________________________________________________
|
| Update clock
|___________________________________________________________________*/

	// Get the current time (# milliseconds since the program started)
    new_time = timeGetTime ();

	// Compute the elapsed time (in milliseconds) since the last time through this loop
    if (last_time == 0) 
    elapsed_time = 0;
    else 
    elapsed_time = new_time - last_time;
    last_time = new_time;

/*____________________________________________________________________
|
| Process user input
|___________________________________________________________________*/

		// Any event ready?
		if (evGetEvent (&event)) {

			// key press?
			if (event.type == evTYPE_RAW_KEY_PRESS)	{

				// If ESC pressed, exit the program
				if (event.keycode == evKY_ESC && g_CurrentState != STATE_HELP)
					quit = TRUE;
                // Screenshot?
                else if (event.keycode == evKY_F1) {
                    take_screenshot = TRUE;
                }

                // --- STATE: TITLE ---
                if (g_CurrentState == STATE_TITLE) {
                        if (event.keycode == evKY_ENTER) {
                            // 1. Transition to the game
                            g_CurrentState = STATE_PLAYING;

                            timeRemaining = gameTimeLimit; // Reset timer to 60 seconds
                            numDucksHit = 0;       // Reset score/targets 
                        }
                        else if (event.keycode == 'h') {
                            // Show help screen
                            g_CurrentState = STATE_HELP;
                        }
                        else if (event.keycode == 'c') {
                            // Show credits screen
                            g_CurrentState = STATE_CREDITS;
                        }

                        // Initialize Ducks in the Square Zone
                        for (int i = 0; i < NUM_DUCKS; i++) {
                            float rx = X_MIN + ((float)rand() / (float)RAND_MAX) * (X_MAX - X_MIN);
                            float rz = Z_MIN + ((float)rand() / (float)RAND_MAX) * (Z_MAX - Z_MIN);

                            duckPosition[i] = { rx, 0.0f, rz }; // Start at ground
                            duckSpeed[i] = 0.01f + ((float)rand() / (float)RAND_MAX) * 0.02f; // Random upward speed
                            duckActive[i] = true;
                        }
                        // Reset score for next round
                        score = 0;
                        numDucksHit = 0;

                    }

                // --- STATE: HELP ---
                else if (g_CurrentState == STATE_HELP) {
                    // Press 'H' or 'ESC' to go back to the title menu
                    if (event.keycode == 'h' || event.keycode == evKY_ESC) {
                        g_CurrentState = STATE_TITLE;
                    }
                }

                // --- STATE: CREDITS ---
                else if (g_CurrentState == STATE_CREDITS) {
                    // Press 'H' or 'ESC' to go back to the title menu
                    if (event.keycode == 'c' || event.keycode == evKY_ESC) {
                        g_CurrentState = STATE_TITLE;
                    }
                }

                // --- STATE: GAMEOVER ---
                else if (g_CurrentState == STATE_GAMEOVER) {
                    if (event.keycode == evKY_ENTER || event.keycode == evKY_ESC) {
                        g_CurrentState = STATE_TITLE; // Go back to start
                    }
                }

                // --- STATE: PLAYING ---
                else if (g_CurrentState == STATE_PLAYING) {
                    if (event.keycode == 'w')
                        cmd_move |= POSITION_MOVE_FORWARD;
                    else if (event.keycode == 's')
                        cmd_move |= POSITION_MOVE_BACK;
                    else if (event.keycode == 'a')
                        cmd_move |= POSITION_MOVE_LEFT;
                    else if (event.keycode == 'd')
                        cmd_move |= POSITION_MOVE_RIGHT;
                    else if (event.keycode == 'f')
                        draw_wireframe = true;
                    }
                }

		        // key release?
		        else if (event.type == evTYPE_RAW_KEY_RELEASE) {
                if (g_CurrentState == STATE_PLAYING) {
                        if (event.keycode == 'w')
                            cmd_move &= ~(POSITION_MOVE_FORWARD);
                        else if (event.keycode == 's')
                            cmd_move &= ~(POSITION_MOVE_BACK);
                        else if (event.keycode == 'a')
                            cmd_move &= ~(POSITION_MOVE_LEFT);
                        else if (event.keycode == 'd')
                            cmd_move &= ~(POSITION_MOVE_RIGHT);
                        else if (event.keycode == 'f')
                            draw_wireframe = false;
                    }
		        }

                boolean walking = false;

                if (cmd_move != 0) {
                    walking = true;
                    if (!snd_IsPlaying(s_footstep))
                        snd_PlaySound(s_footstep, 1);
                        snd_SetSoundVolume(s_footstep, 80);
                    }
                    else {
                        walking = false;
                        snd_StopSound(s_footstep);
                    }

                // Start Music
                if (!snd_IsPlaying(s_soundtrack)) {
                    snd_PlaySound(s_soundtrack, 1);
                    snd_SetSoundVolume(s_soundtrack, 70);
                }

                // mouse left press?
                if (event.type == evTYPE_MOUSE_LEFT_PRESS) {
                    if (g_CurrentState == STATE_PLAYING) {

                        // Perform a ray test on all active ducks
                        gx3dRay viewVector;
                        viewVector.origin = position;
                        viewVector.direction = heading;
                        bool duck_hit = false;
                        int i;

                        for (i = 0; i < NUM_DUCKS && !duck_hit; i++) {
                            if (duckOnScreen[i] && duckActive[i]) {
                                gxRelation rel = gx3d_Relation_Ray_Sphere(&viewVector, &duckSphere[i]);
                                if (rel != gxRELATION_OUTSIDE) {

                                    // Remove this duck from the game (it will respawn in the update loop)
                                    duckActive[i] = false;
                                    numDucksHit++; // Previously numDucksHit
                                    score += 1; // Award 1 points per duck

                                    // Create a new ouch hit marker at the duck's location
                                    ouchPosition[ouchIndex] = duckSphere[i].center;
                                    ouchTimer[ouchIndex] = 1000; // Display for 1 second
                                    ouchIndex = (ouchIndex + 1) % MAX_OUCH;

                                    duck_hit = true;
                                }
                            }
                        }
                }
            }         
		}

if (g_CurrentState == STATE_PLAYING) {
    // Check for camera movement (via mouse)
    msGetMouseMovement(&move_x, &move_y);

/*____________________________________________________________________
|
| Update camera view
|___________________________________________________________________*/

    bool position_changed, camera_changed;
    Position_Update(elapsed_time, cmd_move, move_y, move_x, force_update,
        &position_changed, &camera_changed, &position, &heading);
    //snd_SetListenerPosition (position.x, position.y, position.z, snd_3D_APPLY_NOW);
    //snd_SetListenerOrientation (heading.x, heading.y, heading.z, 0, 1, 0, snd_3D_APPLY_NOW);
}
else {
    // Keep the mouse "cleared" so it doesn't snap the game is started
    msGetMouseMovement(&move_x, &move_y);
}

/*____________________________________________________________________
|
| In-game Timer Logic
|___________________________________________________________________*/
if (g_CurrentState == STATE_PLAYING) {
    timeRemaining -= (int)elapsed_time;

    if (timeRemaining <= 0) {
        timeRemaining = 0;
        g_CurrentState = STATE_GAMEOVER;
    }
}

/*____________________________________________________________________
|
| Duck Spawn Logic
|___________________________________________________________________*/

if (g_CurrentState == STATE_PLAYING) {
    // --- Update Duck Positions & Collision ---
    for (int i = 0; i < NUM_DUCKS; i++) {
        if (duckActive[i]) {
            // Move upward
            duckPosition[i].y += duckSpeed[i] * (float)elapsed_time;

            // If duck flies above the ceiling, reset it to the bottom square spawn
            if (duckPosition[i].y > FLY_CEILING) {
                duckPosition[i].x = X_MIN + ((float)rand() / (float)RAND_MAX) * (X_MAX - X_MIN);
                duckPosition[i].y = 0.0f;
                duckPosition[i].z = Z_MIN + ((float)rand() / (float)RAND_MAX) * (Z_MAX - Z_MIN);
            }

            // Update bounding spheres
            duckSphere[i] = obj_duck->bound_sphere;
            duckSphere[i].center = duckPosition[i];
            duckSphere[i].radius *= 0.5; // Matching the render scale
        }
        else {
            // If duck was shot, respawn it at the bottom after a short delay
            // (Optional: you can add a timer here, or just reset it immediately)
            duckPosition[i].x = X_MIN + ((float)rand() / (float)RAND_MAX) * (X_MAX - X_MIN);
            duckPosition[i].y = 0.0f;
            duckPosition[i].z = Z_MIN + ((float)rand() / (float)RAND_MAX) * (Z_MAX - Z_MIN);
            duckActive[i] = true;
        }
    }
}

/*____________________________________________________________________
|
| Draw 3D graphics
|___________________________________________________________________*/

	// Render the screen
	gx3d_ClearViewport (gx3d_CLEAR_SURFACE | gx3d_CLEAR_ZBUFFER, color, gx3d_MAX_ZBUFFER_VALUE, 0);

    // Start rendering in 3D
    if (gx3d_BeginRender ()) {                                             
        // Set the default material
        gx3d_SetMaterial (&material_default);  

        gx3d_SetAmbientLight(color3d_white);

        // Draw ground
        gx3d_GetTranslateMatrix(&m,0,0,0);
        gx3d_SetObjectMatrix(obj_ground,&m);
        gx3d_SetTexture(0,tex_ground);
        gx3d_DrawObject(obj_ground,0);

        // Draw skydome
        gx3d_GetScaleMatrix(&m1,200,100,200);
        gx3d_GetTranslateMatrix(&m2,0,0,0);
        gx3d_MultiplyMatrix(&m1,&m2,&m);
        gx3d_SetObjectMatrix(obj_skydome,&m);
        gx3d_SetTexture(0,tex_skydome);
        gx3d_DrawObject(obj_skydome,0);

  	    // Enable alpha blending and testing
		gx3d_EnableAlphaBlending ();
		gx3d_EnableAlphaTesting (128);

        // Enable a directional light
		gx3d_EnableLight (dir_light);
        gx3d_SetAmbientLight(color3d_dim);

        // Set lighting for ducks
        gx3d_DisableLight(dir_light);
        gx3d_SetAmbientLight(color3d_white);

        // --- Draw Ducks ---
        static gx3dVector billboard_normal = { 0, 0, 1 };
        for (int i = 0; i < NUM_DUCKS; i++) {
            duckOnScreen[i] = false;
            if (duckActive[i]) {
                gx3d_GetScaleMatrix(&m1, 0.5, 0.5, 0.5);
                gx3d_GetTranslateMatrix(&m3, duckPosition[i].x, duckPosition[i].y, duckPosition[i].z);
                gx3d_MultiplyMatrix(&m1, &m3, &m);
                gx3d_SetObjectMatrix(obj_duck, &m);
                gx3d_SetTexture(0, tex_duck);
                gx3d_DrawObject(obj_duck, 0);
                duckOnScreen[i] = true;
            }
        }

        gx3d_DisableAlphaBlending();

        /*____________________________________________________________________
        |
        | Process ouch markers (Floating Score Markers)
        |___________________________________________________________________*/

        const float OUCH_SCALE = 7;
        // Update ouch markers timers
        for (int i = 0; i < MAX_OUCH; i++) {
            if (ouchTimer[i] > 0)
                ouchTimer[i] -= elapsed_time;
        }

        // Draw ouch markers
        gx3d_EnableAlphaBlending();
        gx3d_EnableAlphaTesting(128);
        for (int i = 0; i < MAX_OUCH; i++) {
            if (ouchTimer[i] > 0) {
                gx3d_GetScaleMatrix(&m1, OUCH_SCALE, OUCH_SCALE, OUCH_SCALE);
                gx3d_GetBillboardRotateYMatrix(&m2, &billboard_normal, &heading);

                // This math makes the marker float upward as its timer runs out
                float upward_float = (1.0f - (ouchTimer[i] / 1000.0f)) * (2.0f * OUCH_SCALE);
                gx3d_GetTranslateMatrix(&m3, ouchPosition[i].x, ouchPosition[i].y + upward_float + 5, ouchPosition[i].z);

                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_MultiplyMatrix(&m, &m3, &m);
                gx3d_SetObjectMatrix(obj_ouch, &m);
                gx3d_SetTexture(0, tex_ouch);
                gx3d_DrawObject(obj_ouch, 0);
            }
        }
        gx3d_DisableAlphaBlending();
        gx3d_DisableAlphaTesting();


        /*____________________________________________________________________
        |
        | Draw 2D graphics on top of 3D
        |___________________________________________________________________*/

        if (g_CurrentState == STATE_PLAYING) {

            // --- SETUP 2D RENDER STATE ---
            gx3d_DisableZBuffer();
            gx3d_EnableAlphaBlending();
            gx3d_EnableAlphaTesting(10);

            // Save current view matrix
            gx3dMatrix view_save;
            gx3d_GetViewMatrix(&view_save);

            // Set new view matrix
            gx3dVector tfrom = { 0,0,-1 }, tto = { 0,0,0 }, twup = { 0,1,0 };
            gx3d_CameraSetPosition(&tfrom, &tto, &twup, gx3d_CAMERA_ORIENTATION_LOOKTO_FIXED);
            gx3d_CameraSetViewMatrix();

            // --- DRAW CROSSHAIR ---
            gx3d_GetScaleMatrix(&m1, 0.03f, 0.03f, 0.03f);
            // Position at the center of the HUD coordinate system
            gx3d_GetTranslateMatrix(&m2, 0, -0.06f, 0);
            gx3d_MultiplyMatrix(&m1, &m2, &m);
            gx3d_SetObjectMatrix(crosshair, &m);
            gx3d_SetTexture(0, tex_crosshair);
            gx3d_DrawObject(crosshair, 0);

            // --- DRAW NUMERICAL SCORE ---
            // Create a local array of the textures we just loaded
            gx3dTexture numberTextures[] = { n0, n1, n2, n3, n4, n5, n6, n7, n8, n9 };

            int tempScore = score;
            int digits[5];
            int numDigits = 0;

            if (tempScore == 0) {
                digits[numDigits++] = 0;
            }
            else {
                while (tempScore > 0 && numDigits < 5) {
                    digits[numDigits++] = tempScore % 10;
                    tempScore /= 10;
                }
            }

            // Draw the digits
            for (int i = 0; i < numDigits; i++) {
                int digitValue = digits[i];

                gx3d_GetScaleMatrix(&m1, 0.03f, 0.05f, 0);
                // Aligning them to the right: 0.5 is right edge, moving left by 0.05 per digit
                gx3d_GetTranslateMatrix(&m2, 0.52f - (0.06f * i), 0.2f, 0);

                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);

                // Use our pointer array to pick the right texture handle
                gx3d_SetTexture(0, numberTextures[digitValue]);
                gx3d_DrawObject(menuButton, 0);
            }

            // Restore view matrix
            gx3d_SetViewMatrix(&view_save);
            gx3d_DisableAlphaTesting();
            gx3d_DisableAlphaBlending();
            gx3d_EnableZBuffer();
        }

/*____________________________________________________________________
|
| 2D OVERLAY RENDERING (Menus & UI)
|___________________________________________________________________*/

// If we are NOT playing, we are in a menu state that needs a 2D overlay
        if (g_CurrentState != STATE_PLAYING) {

            // --- SETUP 2D RENDER STATE ---
            gx3d_DisableZBuffer();
            gx3d_EnableAlphaBlending();
            gx3d_EnableAlphaTesting(128);

            // Save 3D view and set up a Flat 2D view
            gx3dMatrix view_save_menu;
            gx3d_GetViewMatrix(&view_save_menu);
            gx3dVector tfrom = { 0,0,-1 }, tto = { 0,0,0 }, twup = { 0,1,0 };
            gx3d_CameraSetPosition(&tfrom, &tto, &twup, gx3d_CAMERA_ORIENTATION_LOOKTO_FIXED);
            gx3d_CameraSetViewMatrix();

            // --- DRAW MENU CONTENT ---

            if (g_CurrentState == STATE_TITLE) {
                // Draw Title
                gx3d_GetScaleMatrix(&m1, 0.25f, 0.4f, 1.0f);
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.25f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_title);
                gx3d_DrawObject(menuButton, 0);

                // --- DRAW TITLE ---
                gx3d_GetScaleMatrix(&m1, 0.25f, 0.4f, 1.0f);
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.25f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_title);
                gx3d_DrawObject(menuButton, 0);

                // --- DRAW START BUTTON ---
                gx3d_GetScaleMatrix(&m1, 0.15f, 0.15f, 1.0f);
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.16f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_start);
                gx3d_DrawObject(menuButton, 0);

                // --- DRAW HELP BUTTON ---
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.22f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_help);
                gx3d_DrawObject(menuButton, 0);

                // --- DRAW CREDITS BUTTON ---
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.28f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_creditsButton);
                gx3d_DrawObject(menuButton, 0);

                // --- DRAW QUIT BUTTON ---
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.34f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_quit);
                gx3d_DrawObject(menuButton, 0);
            }
            else if (g_CurrentState == STATE_HELP) {
                gx3d_GetScaleMatrix(&m1, 0.25f, 0.4f, 1.0f);
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.4f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_helpInfo);
                gx3d_DrawObject(menuButton, 0);
            }
            else if (g_CurrentState == STATE_CREDITS) {
                gx3d_GetScaleMatrix(&m1, 0.25f, 0.3f, 1.0f);
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.3f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_credits);
                gx3d_DrawObject(menuButton, 0);
            }
            else if (g_CurrentState == STATE_GAMEOVER) {
                // Draw Game Over Header
                gx3d_GetScaleMatrix(&m1, 0.25f, 0.4f, 1.0f);
                gx3d_GetTranslateMatrix(&m2, 0.0f, -0.42f, 0.0f);
                gx3d_MultiplyMatrix(&m1, &m2, &m);
                gx3d_SetObjectMatrix(menuButton, &m);
                gx3d_SetTexture(0, tex_gameover);
                gx3d_DrawObject(menuButton, 0);

                // Note: You can draw the finalScore text here too!
            }

            // --- 3. RESTORE 3D RENDER STATE ---
            gx3d_SetViewMatrix(&view_save_menu);
            gx3d_DisableAlphaTesting();
            gx3d_DisableAlphaBlending();
            gx3d_EnableZBuffer();
        }

	    // Stop rendering
	    gx3d_EndRender ();

        // Take screenshot
        if (take_screenshot) {
            char str[32];
            char buff[50];
            static int screenshot_count = 0;
            strcpy(str, SCREENSHOT_FILENAME);
            strcat(str, itoa(screenshot_count++, buff, 10));
            strcat(str, ".bmp");
            gxWriteBMPFile(str);
        }

	    // Page flip (so user can see it)
	    gxFlipVisualActivePages (FALSE);
	    }   
  }

/*____________________________________________________________________
|
| Free stuff and exit
|___________________________________________________________________*/

    gx3d_FreeObject (obj_skydome);
    gx3d_FreeObject (obj_ground);
    gx3d_FreeObject (obj_duck);
    snd_FreeSound(s_soundtrack);
    snd_FreeSound(s_footstep);
    snd_Free();
}

/*____________________________________________________________________
|
| Function: Init_Render_State
|
| Input: Called from Program_Run()
| Output: Initializes general 3D render state.
|___________________________________________________________________*/

static void Init_Render_State()
{
    // Enable zbuffering
    gx3d_EnableZBuffer ();

    // Enable lighting
    gx3d_EnableLighting ();

    // Set the default alpha blend factor
    gx3d_SetAlphaBlendFactor (gx3d_ALPHABLENDFACTOR_SRCALPHA, gx3d_ALPHABLENDFACTOR_INVSRCALPHA);

    // Init texture addressing mode - wrap in both u and v dimensions
    gx3d_SetTextureAddressingMode (0, gx3d_TEXTURE_DIMENSION_U | gx3d_TEXTURE_DIMENSION_V, gx3d_TEXTURE_ADDRESSMODE_WRAP);
    gx3d_SetTextureAddressingMode (1, gx3d_TEXTURE_DIMENSION_U | gx3d_TEXTURE_DIMENSION_V, gx3d_TEXTURE_ADDRESSMODE_WRAP);
    // Texture stage 0 default blend operator and arguments
    gx3d_SetTextureColorOp (0, gx3d_TEXTURE_COLOROP_MODULATE, gx3d_TEXTURE_ARG_TEXTURE, gx3d_TEXTURE_ARG_CURRENT);
    gx3d_SetTextureAlphaOp (0, gx3d_TEXTURE_ALPHAOP_SELECTARG1, gx3d_TEXTURE_ARG_TEXTURE, 0);
    // Texture stage 1 is off by default
    gx3d_SetTextureColorOp (1, gx3d_TEXTURE_COLOROP_DISABLE, 0, 0);
    gx3d_SetTextureAlphaOp (1, gx3d_TEXTURE_ALPHAOP_DISABLE, 0, 0);
      
    // Set default texture coordinates
    gx3d_SetTextureCoordinates (0, gx3d_TEXCOORD_SET0);
    gx3d_SetTextureCoordinates (1, gx3d_TEXCOORD_SET1);

    // Enable trilinear texture filtering
    gx3d_SetTextureFiltering (0, gx3d_TEXTURE_FILTERTYPE_TRILINEAR, 0);
    gx3d_SetTextureFiltering (1, gx3d_TEXTURE_FILTERTYPE_TRILINEAR, 0);
}

/*____________________________________________________________________
|
| Function: Program_Free
|
| Input: Called from CMainFrame::OnClose()
| Output: Exits graphics mode.
|___________________________________________________________________*/

void Program_Free ()
{
    // Stop event processing 
    evStopEvents ();
    // Return to text mode 
    if (Pgm_system_font)
      gxFreeFont (Pgm_system_font);
    gxStopGraphics ();
}
