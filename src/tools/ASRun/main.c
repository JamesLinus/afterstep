/*
 * Copyright (c) 2005 Sasha Vasko <sasha at aftercode.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "../../../configure.h"
#include "../../../libAfterStep/asapp.h"
#include "../../../libAfterStep/module.h"
#include "../../../libAfterConf/afterconf.h"
#include "../../../libASGTK/asgtk.h"

#define ENTRY_WIDTH		300


typedef struct ASRunState
{
#define ASRUN_ExecInTerm  		(0x01<<0)
#define ASRUN_Persist 			(0x01<<1)
	
	ASFlagType flags ;
	GtkWidget *main_window ;	
	
	GtkWidget *target_combo ;
	GtkWidget *target_entry ;
	GtkWidget *run_in_term_check ;


}ASRunState;

ASRunState AppState ;

void
on_destroy(GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit();
}


void
on_exec_clicked(GtkWidget *widget, gpointer user_data)
{
	if( AppState.target_entry )
	{	
		char *text = stripcpy(gtk_entry_get_text(GTK_ENTRY(AppState.target_entry)));
		if( text != NULL )
		{
			if( text[0] != '\0' ) 
			{	
				if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(AppState.run_in_term_check) ) )
					SendTextCommand ( F_ExecInTerm, NULL, text, 0);
				else
					SendTextCommand ( F_EXEC, NULL, text, 0);
				sleep_a_millisec(500);
				if( !get_flags( AppState.flags, ASRUN_Persist ) )
					gtk_main_quit();
				else
					asgtk_combo_box_add_to_history( GTK_COMBO_BOX(AppState.target_combo), text );
			}
			free( text );
		}
	}
}


void init_ASRun(ASFlagType flags )
{
	
	GtkWidget *main_vbox ;
	GtkWidget *hbox ;
	GtkWidget *frame ;
	GtkWidget *exec_btn, *cancel_btn ;

	memset( &AppState, 0x00, sizeof(AppState));
	
	AppState.flags = flags ;
	AppState.main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  	gtk_window_set_title (GTK_WINDOW (AppState.main_window), MyName);
	/*gtk_window_set_resizable( GTK_WINDOW (AppState.main_window), FALSE);*/
	colorize_gtk_widget( AppState.main_window, get_colorschemed_style_normal() );

	frame = gtk_frame_new( NULL );
	gtk_container_add (GTK_CONTAINER (AppState.main_window), frame);
	gtk_container_set_border_width( GTK_CONTAINER (frame), 5 );
	gtk_widget_show(frame);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), main_vbox);
	gtk_container_set_border_width( GTK_CONTAINER (main_vbox), 5 );

	
	/********   Line 1 *******/
	AppState.run_in_term_check = gtk_check_button_new_with_label("Exec in terminal");
	if( get_flags( AppState.flags, ASRUN_ExecInTerm ) )
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(AppState.run_in_term_check), TRUE );

	hbox = gtk_hbox_new( FALSE, 5 );
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new("Command to execute:"), FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), AppState.run_in_term_check, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	
	/********   Line 2 *******/
	AppState.target_combo = gtk_combo_box_entry_new_text(); 
	gtk_container_set_border_width( GTK_CONTAINER (AppState.target_combo), 1 );
	gtk_widget_set_size_request (AppState.target_combo, ENTRY_WIDTH, -1);
	
	frame = gtk_frame_new( NULL );
	gtk_container_add (GTK_CONTAINER (frame), AppState.target_combo);
	gtk_container_set_border_width( GTK_CONTAINER (frame), 1 );
	gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 5);
	gtk_widget_show_all(frame);
	gtk_widget_show(frame);
	colorize_gtk_edit( AppState.target_combo );
	
	if( GTK_IS_CONTAINER(AppState.target_combo) )
		gtk_container_forall( GTK_CONTAINER(AppState.target_combo), find_combobox_entry, &AppState.target_entry );
	
	/********   Line 3 *******/
	hbox = gtk_hbutton_box_new();
	gtk_box_pack_end (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 5);
	exec_btn = gtk_button_new_from_stock( GTK_STOCK_EXECUTE );
	gtk_box_pack_start (GTK_BOX (hbox), exec_btn, FALSE, FALSE, 0);
	cancel_btn = gtk_button_new_from_stock( GTK_STOCK_CANCEL );
	gtk_box_pack_end (GTK_BOX (hbox), cancel_btn, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	   	
	/********   Callbacks *******/
	/* if above succeeded then path_entry should be not NULL here : */
	/* TODO : insert proper change handlers and data pointers here : */
	if( AppState.target_entry ) 
	{	
		gtk_entry_set_has_frame(  GTK_ENTRY(AppState.target_entry), FALSE );
		g_signal_connect ( G_OBJECT (AppState.target_entry), "activate",
		      			   G_CALLBACK (on_exec_clicked), (gpointer) NULL);
	}

	g_signal_connect (G_OBJECT(AppState.target_combo), "changed",
			  			G_CALLBACK (NULL), (gpointer) NULL);
	
	g_signal_connect (G_OBJECT (cancel_btn), "clicked", G_CALLBACK (on_destroy), NULL);
	g_signal_connect (G_OBJECT (exec_btn), "clicked", G_CALLBACK (on_exec_clicked), NULL);
	
	g_signal_connect (G_OBJECT (AppState.main_window), "destroy", G_CALLBACK (on_destroy), NULL);
	/********   Show them all *******/
	gtk_widget_show_all (main_vbox);

	gtk_widget_show(AppState.main_window);
}	 

int
main (int argc, char *argv[])
{
	ASFlagType flags = 0 ; 
	int i;
	init_asgtkapp( argc, argv, CLASS_ASCP, NULL, 0);
	for( i = 1 ; i < argc ; ++i ) 
	{	
		if( argv[i] == NULL ) 
			continue;
		if( mystrcasecmp( argv[i], "--exec-in-term" ) == 0 )
			set_flags( flags, ASRUN_ExecInTerm );
		else if( mystrcasecmp( argv[i], "--persist" ) == 0 )
			set_flags( flags, ASRUN_Persist );
	}
	ConnectAfterStep(0,0);
	init_ASRun( flags );
  	gtk_main ();
  	return 0;
}
