#ifndef ASINTERNALS_H_HEADER_INCLUDED
#define ASINTERNALS_H_HEADER_INCLUDED

#include "menus.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#include <X11/Xresource.h>
#endif /* SHAPE */

struct FunctionData;
struct ASWindow;
struct ASRectangle;
struct ASDatabase;
struct ASWMProps;
struct MoveResizeData;
struct MenuItem;
struct ASEvent;
struct ASFeel;
struct MyLook;

typedef struct queue_buff_struct
{
  struct queue_buff_struct *next;
  unsigned char            *data;
  int                       size;
  int                       done;
}queue_buff_struct;

typedef struct module_ibuf_t
{
  size_t                len;
  size_t                done;
  Window                window;
  size_t                size;
  char                 *text;
  int                   name_size, text_size;
  struct FunctionData  *func;
  int cont;
}module_ibuf_t;

typedef struct module_t
{
  int                   fd;
  int                   active;
  char                 *name;
  long                  mask;
  queue_buff_struct    *output_queue;
  module_ibuf_t         ibuf;
}module_t;


/******************************************************************/
/* these are global functions and variables private for afterstep */

/* from configure.c */
void error_point();
void tline_error(const char* err_text);
void str_error(const char* err_format, const char* string);

int is_executable_in_path (const char *name);

typedef struct _as_dirs
{
    char* after_dir ;
    char* after_sharedir;
    char* afters_noncfdir;
} ASDirs;

/* from afterstep.c */
extern ASDirs as_dirs;

/* from dirtree.c */
char * strip_whitespace (char *str);

/* from configure.c */
//extern XContext MenuContext;    /* context for afterstep menus */
extern struct ASDatabase    *Database;

/**************************************************************************/
/* Global variables :                                                     */
/**************************************************************************/

#ifdef SHAPE
extern int    ShapeEventBase;
extern int    ShapeErrorBase;
#endif

extern ASFlagType    AfterStepState;              /* see ASS_ flags above */
/* this are linked lists of structures : */
extern struct ASDatabase *Database;
extern ASHashTable       *ComplexFunctions;

/* global variables for Look values : */
extern unsigned long XORvalue;
extern int           RubberBand;
extern char         *RMGeom;
extern int           Xzap, Yzap;
extern int           DrawMenuBorders;
extern int           TextureMenuItemsIndividually;
extern int           StartMenuSortMode;
extern int           ShadeAnimationSteps;

extern int           fd_width, x_fd;

extern struct ASWindow *ColormapWin;

extern struct ASVector *Modules;               /* dynamically resizable array of module_t data structures */
#define MODULES_LIST    VECTOR_HEAD(module_t,*Modules)
#define MODULES_NUM     VECTOR_USED(*Modules)

extern int           Module_fd;
extern int           Module_npipes;

extern Bool   		 menu_event_mask[LASTEvent];  /* menu event filter */

extern int    menuFromFrameOrWindowOrTitlebar;

/**************************************************************************/
/**************************************************************************/
/* Function prototypes :                                                  */
/**************************************************************************/

/*************************** afterstep.c : ********************************/
void Done (Bool restart, char *command);

/*************************** colormaps.c : ********************************/
void SetupColormaps();
void CleanupColormaps();
void InstallWindowColormaps (ASWindow *asw);
void UninstallWindowColormaps (ASWindow *asw);
void InstallRootColormap (void);
void UninstallRootColormap (void);
void InstallAfterStepColormap (void);
void UninstallAfterStepColormap (void);


/*************************** configure.c **********************************/
Bool GetIconFromFile (char *file, MyIcon * icon, int max_colors);
struct ASImage *GetASImageFromFile (char *file);

void InitBase (Bool free_resources);
void InitDatabase (Bool free_resources);

void InitLook (struct MyLook *look, Bool free_resources);
void InitFeel (struct ASFeel *feel, Bool free_resources);

#define PARSE_BASE_CONFIG       (0x01<<0)
#define PARSE_LOOK_CONFIG       (0x01<<1)
#define PARSE_FEEL_CONFIG       (0x01<<2)
#define PARSE_DATABASE_CONFIG   (0x01<<3)
/* some synonims : */
#define BASE_CONFIG_CHANGED     PARSE_BASE_CONFIG
#define LOOK_CONFIG_CHANGED     PARSE_LOOK_CONFIG
#define FEEL_CONFIG_CHANGED     PARSE_FEEL_CONFIG
#define DATABASE_CONFIG_CHANGED PARSE_DATABASE_CONFIG
#define PARSE_EVERYTHING        (0xFFFFFFFF)
#define EVERYTHING_CHANGED      (0xFFFFFFFF)

void LoadASConfig (int thisdesktop, ASFlagType what);
/*************************** decorations.c ********************************/
int check_allowed_function2 (int func, ASHints *hints);
int check_allowed_function (FunctionData *fdata, ASHints *hints);
ASFlagType compile_titlebuttons_mask (ASHints *hints);
int estimate_titlebar_size( ASHints *hints );


/*************************** events.c ********************************/
const char *context2text(int ctx);
void DigestEvent    ( struct ASEvent *event );
void DispatchEvent  ( struct ASEvent *event );
void HandleEvents   ();
void WaitForButtonsUpLoop ();
Bool WaitEventLoop( struct ASEvent *event, int finish_event_type, long timeout );
Bool IsClickLoop( struct ASEvent *event, unsigned int end_mask, unsigned int click_time );
Bool WaitWindowLoop( char *pattern, long timeout );

void AlarmHandler (int nonsense);

Bool KeyboardShortcuts (XEvent * xevent, int return_event, int move_size);

void HandleExpose (struct ASEvent*);
extern void HandleFocusIn (struct ASEvent *event);
extern void HandleFocusOut (struct ASEvent *event);
extern void HandleDestroyNotify (struct ASEvent *event);
extern void HandleMapRequest (struct ASEvent *event);
extern void HandleMapNotify (struct ASEvent *event);
extern void HandleUnmapNotify (struct ASEvent *event);
extern void HandleButtonRelease(struct ASEvent *event);
extern void HandleButtonPress (struct ASEvent *event);
extern void HandleEnterNotify (struct ASEvent *event);
extern void HandleLeaveNotify (struct ASEvent *event);
extern void HandleConfigureRequest (struct ASEvent *event);
extern void HandleClientMessage (struct ASEvent *event);
extern void HandlePropertyNotify (struct ASEvent *event);
extern void HandleKeyPress (struct ASEvent *event);
extern void HandleVisibilityNotify (struct ASEvent *event);
extern void HandleColormapNotify (struct ASEvent *event);

#ifdef SHAPE
void          HandleShapeNotify (struct ASEvent *event);
#endif /* SHAPE */

/*************************** functions.c **********************************/
void SetupFunctionHandlers();
ComplexFunction *get_complex_function( char *name );

void ExecuteFunction (struct FunctionData *data, struct ASEvent *event, int Module);
int  DeferExecution (struct ASEvent *event, int cursor, int FinishEvent);
void QuickRestart (char *what);

/************************* housekeeping.c ********************************/
Bool GrabEm   ( struct ScreenInfo *scr, Cursor cursor );
void UngrabEm ();
Bool StartWarping(struct ScreenInfo *scr, Cursor cursor);
void EndWarping();

void PasteSelection (struct ScreenInfo *scr );

/*************************** icons.c *********************************/
void destroy_asiconbox( ASIconBox **pib );
ASIconBox *get_iconbox( int desktop );
Bool add_iconbox_icon( ASWindow *asw );
Bool remove_iconbox_icon( ASWindow *asw );
Bool change_iconbox_icon_desk( ASWindow *asw, int from_desk, int to_desk );
void on_icon_changed( ASWindow *asw );
void rearrange_iconbox_icons( int desktop );


/*************************** menus.c *********************************/

/*************************** menuitem.c *********************************/
int parse_modifier( char *tline );
FunctionData *String2Func ( const char *string, FunctionData *p_fdata, Bool quiet );
void ParsePopupEntry (char *tline, FILE * fd, char **junk, int *junk2);


/*************************** misc.c *********************************/
inline void ungrab_window_buttons( Window w );
inline void ungrab_window_keys (Window w );
void MyXGrabButton ( unsigned button, unsigned modifiers,
                Window grab_window, Bool owner_events, unsigned event_mask,
                int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor);
void MyXUngrabButton ( unsigned button, unsigned modifiers, Window grab_window);
void grab_window_buttons (Window w, ASFlagType context_mask);
void grab_window_keys (Window w, ASFlagType context_mask);
void grab_focus_click( Window w );
void ungrab_focus_click( Window w );
void SetTimer (int delay);


/***************************** module.c ***********************************/
void SetupModules(void);

void ExecModule (char *action, Window win, int context);
int  AcceptModuleConnection (int socket_fd);

void SendPacket (int channel, unsigned long  msg_type, unsigned long num_datum, ...);
void SendVector (int channel, unsigned long  msg_type, struct ASVector *vector);
void SendConfig (int module, unsigned long event_type, ASWindow * t);
void SendString (int module, unsigned long event_type,
               unsigned long data1, unsigned long data2, unsigned long data3, char *name);
/* simplified specialized interface to above functions : */
void broadcast_focus_change( ASWindow *focused );
void broadcast_window_name( ASWindow *asw );
void broadcast_icon_name( ASWindow *asw );
void broadcast_res_names( ASWindow *asw );
void broadcast_status_change( int message, ASWindow *asw );
void broadcast_config (unsigned long event_type, ASWindow * t);

void HandleModuleInOut(unsigned int channel, Bool has_input, Bool has_output);

void KillModuleByName (char *name);
void DeadPipe (int nonsense);
void ShutdownModules();

void RunCommand (FunctionData * fdata, unsigned int channel, Window w);

/******************************* outline.c ********************************/
void MoveOutline( struct MoveResizeData * pdata );

/******************************* pager.c ***********************************/
/* we use 4 windows that are InputOnly and therefore are invisible on the
 * sides of the screen to steal mouse events and allow for virtual viewport
 * move when cursor reaches edge of the screen. :*/
void MoveViewport (int newx, int newy, Bool grab);
void HandlePaging (int HorWarpSize, int VertWarpSize, int *xl,
                   int *yt, int *delta_x, int *delta_y, Bool Grab, struct ASEvent *event);
void ChangeDesks (int new_desk);

#endif /* ASINTERNALS_H_HEADER_INCLUDED */
