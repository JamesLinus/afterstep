#ifndef ASWALLPAPER_INTERFACE_H_HEADER_INCLUDED
#define ASWALLPAPER_INTERFACE_H_HEADER_INCLUDED
/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */

typedef struct ASWallpaperState
{
#define DISPLAY_SYSTEM_BACKS	(0x01<<0)  /* otherwise display private */	
	
	ASFlagType flags ; 
	
	GtkWidget   *main_window ; 

	GtkWidget   *list_hbox;	
	GtkTreeView *backs_list ;
	GtkTreeModel *list_model ;
	
	GtkWidget    *list_add_button ;
	GtkWidget    *list_del_button ;
	GtkWidget    *list_apply_button ;
	
	GtkWidget    *list_preview_container ;
	GtkImage	 *list_preview ; 
	GtkWidget    *make_xml_button ;

	ASImageListEntry *private_backs_list ;
	int 			  private_backs_count;
	
}ASWallpaperState;

extern ASWallpaperState WallpaperState ;

void init_ASWallpaper();

GtkWidget* create_filechooserdialog2 (void);

#endif