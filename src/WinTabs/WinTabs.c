/*
 * This is the complete from scratch rewrite of the original WinList
 * module.
 *
 * Copyright (C) 2003 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*#define DO_CLOCKING      */
#define LOCAL_DEBUG
#define EVENT_TRACE

#include "../../configure.h"

#include "../../libAfterStep/asapp.h"

#include <signal.h>
#include <unistd.h>

#include "../../libAfterImage/afterimage.h"

#include "../../libAfterStep/afterstep.h"
#include "../../libAfterStep/screen.h"
#include "../../libAfterStep/module.h"
#include "../../libAfterStep/mystyle.h"
#include "../../libAfterStep/mystyle_property.h"
#include "../../libAfterStep/parser.h"
#include "../../libAfterStep/clientprops.h"
#include "../../libAfterStep/wmprops.h"
#include "../../libAfterStep/decor.h"
#include "../../libAfterStep/aswindata.h"
#include "../../libAfterStep/balloon.h"
#include "../../libAfterStep/event.h"

#include "../../libAfterConf/afterconf.h"

/**********************************************************************/
/*  AfterStep specific global variables :                             */
/**********************************************************************/
/**********************************************************************/
/*  Gadget local variables :                                         */
/**********************************************************************/
typedef struct ASWinTab
{
	char 			*name ;
	Window 			 client ;

	ASTBarData 		*bar ;
	ASCanvas 		*client_canvas ;

}ASWinTab;

typedef struct {
    Window main_window, tabs_window ;
	ASCanvas *main_canvas ;
	ASCanvas *tabs_canvas ;

	wild_reg_exp *pattern_wrexp ;

	ASVector *tabs ;

	int rows ;
	int row_height ;

}ASWinTabsState ;

ASWinTabsState WinTabsState = { 0 };


#define WINTABS_TAB_EVENT_MASK    (ButtonReleaseMask | ButtonPressMask | \
	                               LeaveWindowMask   | EnterWindowMask | \
                                   StructureNotifyMask )

/**********************************************************************/
/**********************************************************************/
/* Our configuration options :                                        */
/**********************************************************************/
/*char *default_unfocused_style = "unfocused_window_style";
 char *default_focused_style = "focused_window_style";
 char *default_sticky_style = "sticky_window_style";
 */

WinTabsConfig *Config = NULL ;
/**********************************************************************/

void CheckConfigSanity();
void GetBaseOptions (const char *filename);
void GetOptions (const char *filename);
void HandleEvents();
void process_message (unsigned long type, unsigned long *body);
void DispatchEvent (ASEvent * Event);
Window make_wintabs_window();
Window make_tabs_window( Window parent );
void check_swallow_window( ASWindowData *wd );
void rearrange_tabs();
void render_tabs( Bool canvas_resized );
void on_destroy_notify(Window w);

int
main( int argc, char **argv )
{
    /* Save our program name - for error messages */
    InitMyApp (CLASS_GADGET, argc, argv, NULL, NULL, 0 );

    set_signal_handler( SIGSEGV );

    ConnectX( &Scr, PropertyChangeMask|EnterWindowMask );
    ConnectAfterStep (  M_END_WINDOWLIST |
                    	WINDOW_CONFIG_MASK |
                    	WINDOW_NAME_MASK );
    balloon_init (False);
    Config = CreateWinTabsConfig ();

    LoadBaseConfig ( GetBaseOptions);
	LoadColorScheme();
	LoadConfig ("wintabs", GetOptions);
    CheckConfigSanity();

	if( Config->pattern != NULL )
	{
		WinTabsState.pattern_wrexp = compile_wild_reg_exp( Config->pattern ) ;
	}else
	{
		show_error( "Empty Pattern requested for windows to be captured and tabbed - Abborting!");
		DeadPipe(0);
	}

	SendInfo ("Send_WindowList", 0);

	WinTabsState.tabs = create_asvector( sizeof(ASWinTab) );

    WinTabsState.main_window = make_wintabs_window();
    WinTabsState.main_canvas = create_ascanvas_container( WinTabsState.main_window );
    WinTabsState.tabs_window = make_tabs_window( WinTabsState.main_window );
    WinTabsState.tabs_canvas = create_ascanvas( WinTabsState.tabs_window );
    map_canvas_window( WinTabsState.tabs_canvas, True );
    set_root_clip_area(WinTabsState.main_canvas );
    map_canvas_window( WinTabsState.main_canvas, True );
    /* final cleanup */
	XFlush (dpy);
	sleep (1);								   /* we have to give AS a chance to spot us */

	/* And at long last our main loop : */
    HandleEvents();
	return 0 ;
}

void HandleEvents()
{
    ASEvent event;
    Bool has_x_events = False ;
    while (True)
    {
        while((has_x_events = XPending (dpy)))
        {
            if( ASNextEvent (&(event.x), True) )
            {
                event.client = NULL ;
                setup_asevent_from_xevent( &event );
                DispatchEvent( &event );
            }
        }
        module_wait_pipes_input ( process_message );
    }
}

void
DeadPipe (int nonsense)
{
    FreeMyAppResources();

    if( WinTabsState.main_canvas )
        destroy_ascanvas( &WinTabsState.main_canvas );
    if( WinTabsState.main_window )
        XDestroyWindow( dpy, WinTabsState.main_window );
    if( Config )
        DestroyWinTabsConfig(Config);

#ifdef DEBUG_ALLOCS
    print_unfreed_mem ();
#endif /* DEBUG_ALLOCS */

    XFlush (dpy);			/* need this for SetErootPixmap to take effect */
	XCloseDisplay (dpy);		/* need this for SetErootPixmap to take effect */
    exit (0);
}

void
CheckConfigSanity()
{
    int i ;
    char *default_style = safemalloc( 1+strlen(MyName)+1);
	default_style[0] = '*' ;
	strcpy( &(default_style[1]), MyName );

    if( Config == NULL )
        Config = CreateWinTabsConfig ();

    Config->gravity = NorthWestGravity ;
    if( get_flags(Config->geometry.flags, XNegative) )
        Config->gravity = get_flags(Config->geometry.flags, YNegative)? SouthEastGravity:NorthEastGravity;
    else if( get_flags(Config->geometry.flags, YNegative) )
        Config->gravity = SouthWestGravity;

    Config->anchor_x = get_flags( Config->geometry.flags, XValue )?Config->geometry.x:0;
    if( get_flags(Config->geometry.flags, XNegative) )
        Config->anchor_x += Scr.MyDisplayWidth ;

    Config->anchor_y = get_flags( Config->geometry.flags, YValue )?Config->geometry.y:0;
    if( get_flags(Config->geometry.flags, YNegative) )
        Config->anchor_y += Scr.MyDisplayHeight ;

    mystyle_get_property (Scr.wmprops);

    Scr.Look.MSWindow[BACK_UNFOCUSED] = mystyle_find( Config->unfocused_style );
    Scr.Look.MSWindow[BACK_FOCUSED] = mystyle_find( Config->focused_style );
    Scr.Look.MSWindow[BACK_STICKY] = mystyle_find( Config->sticky_style );

    for( i = 0 ; i < BACK_STYLES ; ++i )
    {
        static char *default_window_style_name[BACK_STYLES] ={"focused_window_style","unfocused_window_style","sticky_window_style", NULL};
        if( Scr.Look.MSWindow[i] == NULL )
            Scr.Look.MSWindow[i] = mystyle_find( default_window_style_name[i] );
        if( Scr.Look.MSWindow[i] == NULL )
            Scr.Look.MSWindow[i] = mystyle_find_or_default( default_style );
    }
    free( default_style );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    PrintWinTabsConfig (Config);
    Print_balloonConfig ( Config->balloon_conf );
#endif
    balloon_config2look( &(Scr.Look), Config->balloon_conf );
    LOCAL_DEBUG_OUT( "balloon mystyle = %p (\"%s\")", Scr.Look.balloon_look->style,
                    Scr.Look.balloon_look->style?Scr.Look.balloon_look->style->name:"none" );
    set_balloon_look( Scr.Look.balloon_look );

}


void
GetBaseOptions (const char *filename)
{
	ReloadASEnvironment( NULL, NULL, NULL, False );
}

void
GetOptions (const char *filename)
{
    START_TIME(option_time);
    WinTabsConfig *config = ParseWinTabsOptions( filename, MyName );

#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
    PrintWinTabsConfig (config);
#endif
    /* Need to merge new config with what we have already :*/
    /* now lets check the config sanity : */
    /* mixing set and default flags : */
    Config->flags = (config->flags&config->set_flags)|(Config->flags & (~config->set_flags));
    Config->set_flags |= config->set_flags;

    Config->gravity = NorthWestGravity ;
    if( get_flags(config->set_flags, WINTABS_Geometry) )
        merge_geometry(&(config->geometry), &(Config->geometry) );

    if( config->pattern )
    {    
        set_string_value( &(Config->pattern), mystrdup(config->pattern), NULL, 0 );
        Config->pattern_type = config->pattern_type ; 
    }
    if( get_flags(config->set_flags, WINTABS_MaxRows) )
        Config->max_rows = config->max_rows;
    if( get_flags(config->set_flags, WINTABS_MaxColumns) )
        Config->max_columns = config->max_columns;
    if( get_flags(config->set_flags, WINTABS_MinTabWidth) )
        Config->min_tab_width = config->min_tab_width;
    if( get_flags(config->set_flags, WINTABS_MaxTabWidth) )
        Config->max_tab_width = config->max_tab_width;

    if( config->unfocused_style )
        set_string_value( &(Config->unfocused_style), mystrdup(config->unfocused_style), NULL, 0 );
    if( config->focused_style )
        set_string_value( &(Config->focused_style), mystrdup(config->focused_style), NULL, 0 );
    if( config->sticky_style )
        set_string_value( &(Config->sticky_style), mystrdup(config->sticky_style), NULL, 0 );

    if( get_flags(config->set_flags, WINTABS_Align) )
        Config->name_aligment = config->name_aligment;
    if( get_flags(config->set_flags, WINTABS_FBevel) )
        Config->fbevel = config->fbevel;
    if( get_flags(config->set_flags, WINTABS_UBevel) )
        Config->ubevel = config->ubevel;
    if( get_flags(config->set_flags, WINTABS_SBevel) )
        Config->sbevel = config->sbevel;

    if( get_flags(config->set_flags, WINTABS_FCM) )
        Config->fcm = config->fcm;
    if( get_flags(config->set_flags, WINTABS_UCM) )
        Config->ucm = config->ucm;
    if( get_flags(config->set_flags, WINTABS_SCM) )
        Config->scm = config->scm;

    if( get_flags(config->set_flags, WINTABS_H_SPACING) )
        Config->h_spacing = config->h_spacing;
    if( get_flags(config->set_flags, WINTABS_V_SPACING) )
        Config->v_spacing = config->v_spacing;

    if( Config->balloon_conf )
        Destroy_balloonConfig( Config->balloon_conf );
    Config->balloon_conf = config->balloon_conf ;
    config->balloon_conf = NULL ;

    if (config->style_defs)
        ProcessMyStyleDefinitions (&(config->style_defs));
    SHOW_TIME("Config parsing",option_time);
}

/****************************************************************************/
/* PROCESSING OF AFTERSTEP MESSAGES :                                       */
/****************************************************************************/
void
process_message (unsigned long type, unsigned long *body)
{
    LOCAL_DEBUG_OUT( "received message %lX", type );
	if( (type&WINDOW_PACKET_MASK) != 0 )
	{
		struct ASWindowData *wd = fetch_window_by_id( body[0] );
        WindowPacketResult res ;
        /* saving relevant client info since handle_window_packet could destroy the actuall structure */
#if defined(LOCAL_DEBUG) && !defined(NO_DEBUG_OUTPUT)
        Window               saved_w = (wd && wd->canvas)?wd->canvas->w:None;
        int                  saved_desk = wd?wd->desk:INVALID_DESK;
        struct ASWindowData *saved_wd = wd ;
#endif
        LOCAL_DEBUG_OUT( "message %lX window %lX data %p", type, body[0], wd );
        res = handle_window_packet( type, body, &wd );
        LOCAL_DEBUG_OUT( "\t res = %d, data %p", res, wd );
        if( res == WP_DataCreated || res == WP_DataChanged )
        {
            check_swallow_window( wd );
        }else if( res == WP_DataDeleted )
        {
            LOCAL_DEBUG_OUT( "client deleted (%p)->window(%lX)->desk(%d)", saved_wd, saved_w, saved_desk );
        }
    }
}

void
DispatchEvent (ASEvent * event)
{
/*    ASWindowData *pointer_wd = NULL ; */
    ASCanvas *mc = WinTabsState.main_canvas ;

    SHOW_EVENT_TRACE(event);

    if( (event->eclass & ASE_POINTER_EVENTS) != 0 )
    {
    }
    LOCAL_DEBUG_OUT( "mc.geom = %dx%d%+d%+d", mc->width, mc->height, mc->root_x, mc->root_y );
    switch (event->x.type)
    {
	    case ConfigureNotify:
            {
                if( event->w == WinTabsState.main_window ) 
                {                        
                    ASFlagType changes = handle_canvas_config( WinTabsState.main_canvas );
                    if( get_flags( changes, CANVAS_RESIZED ) )
                        rearrange_tabs();
                  
                    if( changes != 0 ) 
                        set_root_clip_area( WinTabsState.main_canvas );
                }else if( event->w == WinTabsState.tabs_window ) 
                {
                    ASFlagType changes = handle_canvas_config( WinTabsState.tabs_canvas );
                    if( get_flags( changes, CANVAS_RESIZED ) )
                    {
                        render_tabs( True );
                    }    
                }
            }
	        break;
        case ButtonPress:
            break;
        case ButtonRelease:
			break;
        case EnterNotify :
			if( event->x.xcrossing.window == Scr.Root )
			{
				withdraw_active_balloon();
				break;
			}
        case LeaveNotify :
        case MotionNotify :
            break ;
        case DestroyNotify:
            on_destroy_notify(event->w);
            break;

        case ClientMessage:
            if ((event->x.xclient.format == 32) &&
                (event->x.xclient.data.l[0] == _XA_WM_DELETE_WINDOW))
			{
                DeadPipe(0);
			}
	        break;
	    case PropertyNotify:
			LOCAL_DEBUG_OUT( "property %s(%lX), _XROOTPMAP_ID = %lX, event->w = %lX, root = %lX", XGetAtomName(dpy, event->x.xproperty.atom), event->x.xproperty.atom, _XROOTPMAP_ID, event->w, Scr.Root );
            if( event->x.xproperty.atom == _XROOTPMAP_ID && event->w == Scr.Root )
            {
                LOCAL_DEBUG_OUT( "root background updated!%s","");
                safe_asimage_destroy( Scr.RootImage );
                Scr.RootImage = NULL ;
            }else if( event->x.xproperty.atom == _AS_STYLE )
			{
				LOCAL_DEBUG_OUT( "AS Styles updated!%s","");
				handle_wmprop_event (Scr.wmprops, &(event->x));
				mystyle_list_destroy_all(&(Scr.Look.styles_list));
				LoadColorScheme();
				CheckConfigSanity();
				/* now we need to update everything */
			}
			break;
    }
}

/********************************************************************/
/* showing our main window :                                        */
/********************************************************************/
Window
make_wintabs_window()
{
	Window        w;
	XSizeHints    shints;
	ExtendedWMHints extwm_hints ;
	int x, y ;
    unsigned int width = max(Config->geometry.width,1);
    unsigned int height = max(Config->geometry.height,1);

    XSetWindowAttributes attributes;
    attributes.background_pixmap = ParentRelative;
    switch( Config->gravity )
	{
		case NorthEastGravity :
            x = Config->anchor_x - width ;
			y = Config->anchor_y ;
			break;
		case SouthEastGravity :
            x = Config->anchor_x - width;
            y = Config->anchor_y - height;
			break;
		case SouthWestGravity :
			x = Config->anchor_x ;
            y = Config->anchor_y - height;
			break;
		case NorthWestGravity :
		default :
			x = Config->anchor_x ;
			y = Config->anchor_y ;
			break;
	}
    LOCAL_DEBUG_OUT( "creating main window with geometry %dx%d%+d%+d", width, height, x, y );
    w = create_visual_window( Scr.asv, Scr.Root, x, y, width, height, 0, InputOutput, CWBackPixmap, &attributes);
    set_client_names( w, "WinTabs", "WINTABS", CLASS_GADGET, MyName );

    shints.flags = USPosition|USSize|PWinGravity;
    shints.win_gravity = Config->gravity ;

	extwm_hints.pid = getpid();
	extwm_hints.flags = EXTWM_PID|EXTWM_StateSkipTaskbar|EXTWM_StateSkipPager|EXTWM_TypeDock ;

	set_client_hints( w, NULL, &shints, AS_DoesWmDeleteWindow, &extwm_hints );

    /* we will need to wait for PropertyNotify event indicating transition
	   into Withdrawn state, so selecting event mask: */
    XSelectInput (dpy, w, PropertyChangeMask|StructureNotifyMask|
                          ButtonPressMask|ButtonReleaseMask|PointerMotionMask|
                          KeyPressMask|KeyReleaseMask|
                          EnterWindowMask|LeaveWindowMask|
						  SubstructureRedirectMask);

	return w ;
}

Window
make_tabs_window( Window parent )
{
	static XSetWindowAttributes attr ;
    Window w ;
	attr.event_mask = WINTABS_TAB_EVENT_MASK ;
    w = create_visual_window( Scr.asv, parent, 0, 0, 1, 1, 0, InputOutput, CWEventMask, &attr );
    return w;
}
/**************************************************************************
 * add/remove a tab code
 **************************************************************************/
void
set_tab_look( ASWinTab *aswt )
{
	set_astbar_hilite( aswt->bar, BAR_STATE_UNFOCUSED, Config->ubevel );
	set_astbar_hilite( aswt->bar, BAR_STATE_FOCUSED,   Config->fbevel );
	set_astbar_composition_method( aswt->bar, BAR_STATE_UNFOCUSED, Config->ucm );
	set_astbar_composition_method( aswt->bar, BAR_STATE_FOCUSED,   Config->fcm );
	set_astbar_style_ptr (aswt->bar, BAR_STATE_UNFOCUSED, Scr.Look.MSWindow[BACK_UNFOCUSED]);
	set_astbar_style_ptr (aswt->bar, BAR_STATE_FOCUSED,   Scr.Look.MSWindow[BACK_FOCUSED]);
}

ASWinTab *
add_tab( Window client, const char *name, INT32 encoding )
{
	ASWinTab aswt ;

	if( AS_ASSERT(client) )
		return NULL ;
	aswt.client = client ;
	aswt.name = mystrdup(name);

	aswt.bar = create_astbar();
	set_tab_look( &aswt );
	add_astbar_label( aswt.bar, 0, 0, 0, Config->name_aligment, Config->h_spacing, Config->v_spacing, name, encoding);

	append_vector( WinTabsState.tabs, &aswt, 1 );

    return PVECTOR_TAIL(ASWinTab,WinTabsState.tabs)-1;
}

void
delete_tab( int index ) 
{
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    destroy_astbar( &(tabs[index].bar) );
    destroy_ascanvas( &(tabs[index].client_canvas) );
    if( tabs[index].name ) 
        free( tabs[index].name );
    vector_remove_index( WinTabsState.tabs, index );
}    

void
place_tabs_line( ASWinTab *tabs, int y, int first, int last, int spare, int max_width, int tab_height )
{
    int i ;
    int x = 0;
    int delta = spare / (last+1-first) ;

    for( i = first ; i <= last ; ++i ) 
    {
        int width  = calculate_astbar_width( tabs[i].bar );
        if( width < Config->min_tab_width ) 
            width = Config->min_tab_width ;
        if( width > max_width )
            width = max_width ;
        width += delta ;
        spare -= delta ; 
        if( i < last ) 
            delta = spare / (last - i) ;

        set_astbar_size( tabs[i].bar, width, tab_height );
        move_astbar( tabs[i].bar, WinTabsState.tabs_canvas, x, y );
        x += width ;
    }    

    
}

void
rearrange_tabs()
{
    int tab_height = 0 ;
	ASCanvas *mc = WinTabsState.main_canvas ;
	int i ;
	ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    int tabs_num = PVECTOR_USED(WinTabsState.tabs) ;
    int x = 0, y = 0 ; 
    int max_x = mc->width ; 
    int max_y = mc->height ; 
    int max_width = Config->max_tab_width ; 
    int start = 0 ;

    if( max_width <= 0 || max_width > max_x )
        max_width = max_x ;

    LOCAL_DEBUG_OUT( "max_x = %d, max_y = %d, max_width = %d", max_x, max_y, max_width );
    i = tabs_num ;
	while( --i >= 0 )
	{
        int height = calculate_astbar_height( tabs[i].bar );
        if( height > tab_height )
            tab_height = height ;
    }

    if( tab_height == 0 || max_x <= 0 || max_y <= 0 )
        return ;
    i = tabs_num ; 
    for( i = 0 ; i < tabs_num ; ++i ) 
    {    
        int width  = calculate_astbar_width( tabs[i].bar );
        if( width < Config->min_tab_width ) 
            width = Config->min_tab_width ;
        if( width > max_width )
            width = max_width ;
        if( x + width > max_x )
        {    
            place_tabs_line( tabs, y, start, i - 1, max_x - x, max_width, tab_height );
            if( y + tab_height > max_y ) 
                break;
            y += tab_height ; 
            x = 0 ;
        }else if( i == tabs_num - 1 )
        {    
            place_tabs_line( tabs, y, start, i, max_x - (x+width), max_width, tab_height );
            x = 0 ;
        }else
            x += width ;
    }
    if( i >= tabs_num )    
        y += tab_height ; 
    
    if( (moveresize_canvas( WinTabsState.tabs_canvas, 0, 0, max_x, y ) & CANVAS_RESIZED) == 0 ) 
        render_tabs( False );
    
    max_y -= y ;
    i = tabs_num ; 
    LOCAL_DEBUG_OUT( "moveresaizing %d client canvases to %dx%d%+d%+d", i, max_x, max_y, 0, y );
    while( --i >= 0 ) 
    {    
        moveresize_canvas( tabs[i].client_canvas, 0, y, max_x, max_y );    
    }
}

void
render_tabs( Bool canvas_resized )
{        
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    int i = PVECTOR_USED(WinTabsState.tabs) ;

    while( --i >= 0  )
    {
        register ASTBarData   *tbar = tabs[i].bar ;
        if( tbar != NULL )
            if( DoesBarNeedsRendering(tbar) || canvas_resized )
                render_astbar( tbar, WinTabsState.tabs_canvas );
    }
    if( is_canvas_dirty( WinTabsState.tabs_canvas ) )
	{
        update_canvas_display( WinTabsState.tabs_canvas );
        update_canvas_display_mask (WinTabsState.tabs_canvas, True);
	}
}

/**************************************************************************
 * Swallowing code
 **************************************************************************/
void
send_swallowed_configure_notify(ASWinTab *aswt)
{
    if( aswt->client_canvas )
    {
		send_canvas_configure_notify(WinTabsState.main_canvas, aswt->client_canvas );
    }
}


void
check_swallow_window( ASWindowData *wd )
{
    Window w;
    int try_num = 0 ;
    ASCanvas *nc ;
    char *name = NULL ;
	INT32 encoding ;
	ASWinTab *aswt = NULL ;
	int i = 0;

    if( wd == NULL && !get_flags( wd->state_flags, AS_Mapped))
        return;

	/* first lets check if we have already swallowed this one : */
	i = PVECTOR_USED(WinTabsState.tabs);
	aswt = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
	while( --i >= 0 )
		if( aswt[i].client == wd->client )
			return ;

	/* now lets try and match its name : */
	name = get_window_name( wd, Config->pattern_type, &encoding );
    LOCAL_DEBUG_OUT( "name(\"%s\")->icon_name(\"%s\")->res_class(\"%s\")->res_name(\"%s\")",
                     wd->window_name, wd->icon_name, wd->res_class, wd->res_name );
	if( match_wild_reg_exp( name, WinTabsState.pattern_wrexp) != 0 )
		return ;

	/* we have a match */
	/* now we actually swallow the window : */
    XGrabServer( dpy );
    /* first lets check if window is still not swallowed : it should have no more then 2 parents before root */
    w = get_parent_window( wd->client );
    LOCAL_DEBUG_OUT( "first parent %lX, root %lX", w, Scr.Root );
	while( w == Scr.Root && ++try_num  < 10 )
	{/* we should wait for AfterSTep to complete AddWindow protocol */
	    /* do the actuall swallowing here : */
    	XUngrabServer( dpy );
		sleep_a_millisec(200*try_num);
		XGrabServer( dpy );
		w = get_parent_window( wd->client );
		LOCAL_DEBUG_OUT( "attempt %d:first parent %lX, root %lX", try_num, w, Scr.Root );
	}
	if( w == Scr.Root )
	{
		XUngrabServer( dpy );
		return ;
	}
    if( w != None )
        w = get_parent_window( w );
    LOCAL_DEBUG_OUT( "second parent %lX, root %lX", w, Scr.Root );
    if( w != Scr.Root )
	{
		XUngrabServer( dpy );
		return ;
	}
    /* its ok - we can swallow it now : */
    /* create swallow object : */
	name = get_window_name( wd, ASN_Name, &encoding );
	aswt = add_tab( wd->client, name, encoding );
    LOCAL_DEBUG_OUT( "crerated new #%d for window \"%s\" client = %8.8lX", PVECTOR_USED(WinTabsState.tabs), name, wd->client );

	if( aswt == NULL )
	{
		XUngrabServer( dpy );
		return ;
	}

    /* first thing - we reparent window and its icon if there is any */
    nc = aswt->client_canvas = create_ascanvas_container( wd->client );
    XReparentWindow( dpy, wd->client, WinTabsState.main_window, WinTabsState.main_canvas->width - nc->width, WinTabsState.main_canvas->height - nc->height );
    XSelectInput (dpy, wd->client, StructureNotifyMask);

#if 0   /* TODO : implement support for icons : */
    if( get_flags( wd->flags, AS_ClientIcon ) && !get_flags( wd->flags, AS_ClientIconPixmap) &&
		wd->icon != None )
    {
        ASCanvas *ic = create_ascanvas_container( wd->icon );
        aswb->swallowed->iconic = ic;
        XReparentWindow( dpy, wd->icon, aswb->canvas->w, (aswb->canvas->width-ic->width)/2, (aswb->canvas->height-ic->height)/2 );
        register_object( wd->icon, (ASMagic*)aswb );
        XSelectInput (dpy, wd->icon, StructureNotifyMask);
        grab_swallowed_canvas_btns(  ic, (aswb->folder!=NULL), withdraw_btn && aswb->parent == WharfState.root_folder);
    }
    aswb->swallowed->current = ( get_flags( wd->state_flags, AS_Iconic ) &&
                                    aswb->swallowed->iconic != NULL )?
                                aswb->swallowed->iconic:aswb->swallowed->normal;
    LOCAL_DEBUG_OUT( "client(%lX)->icon(%lX)->current(%lX)", wd->client, wd->icon, aswb->swallowed->current->w );

#endif
    handle_canvas_config( nc );

    map_canvas_window( nc, True );
    send_swallowed_configure_notify(aswt);

    rearrange_tabs();

    ASSync(False);
    XUngrabServer( dpy );
}

void 
on_destroy_notify(Window w)
{
    ASWinTab *tabs = PVECTOR_HEAD(ASWinTab,WinTabsState.tabs);
    int i = PVECTOR_USED(WinTabsState.tabs) ;
    while( --i >= 0 ) 
        if( tabs[i].client == w ) 
        {
            delete_tab( i );
            rearrange_tabs();
            return ; 
        }    
}    