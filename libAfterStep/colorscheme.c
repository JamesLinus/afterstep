#define LOCAL_DEBUG
#undef DO_CLOCKING

#include "../configure.h"

#include "asapp.h"
#include "afterstep.h"
#include "../libAfterImage/afterimage.h"
#include "colorscheme.h"

ARGB32
make_color_scheme_argb( CARD32 base_alpha16, CARD32 hue360, CARD32 sat100, CARD32 val100 )
{
	CARD32 red16, green16, blue16 ;
	ARGB32 argb ;

	hsv2rgb(degrees2hue16(hue360), percent2val16(sat100), percent2val16(val100), &red16, &green16, &blue16);
	argb = 	MAKE_ARGB32_CHAN16(base_alpha16,ARGB32_ALPHA_CHAN)|
			MAKE_ARGB32_CHAN16(red16,ARGB32_RED_CHAN)|
			MAKE_ARGB32_CHAN16(green16,ARGB32_GREEN_CHAN)|
			MAKE_ARGB32_CHAN16(blue16,ARGB32_BLUE_CHAN);
	return argb;
}

Bool
is_light_hsv( int hue360, int sat100, int val100 )
{
	CARD32 red16, green16, blue16 ;

	hsv2rgb(degrees2hue16(hue360), percent2val16(sat100), percent2val16(val100), &red16, &green16, &blue16);
	return ASCS_BLACK_O_WHITE_CRITERIA16( red16, green16, blue16 );
}

inline void
make_grad_argb( ARGB32 *grad, ARGB32 base_alpha16, int hue360, int sat100, int val100 )
{
	grad[0] = make_color_scheme_argb( base_alpha16, hue360, sat100, val100-ASCS_GRADIENT_BRIGHTNESS_OFFSET );
	grad[1] = make_color_scheme_argb( base_alpha16, hue360, sat100, val100+ASCS_GRADIENT_BRIGHTNESS_OFFSET );
}

ASColorScheme *
make_ascolor_scheme( ARGB32 base, int angle )
{
	ASColorScheme *cs = safecalloc( 1, sizeof(ASColorScheme));
	CARD32 hue16, sat16, val16 ;
	CARD32 base_alpha16 ;
	int sat, val;

	angle = FIT_IN_RANGE( ASCS_MIN_ANGLE, angle, ASCS_MAX_ANGLE );
	cs->angle = angle ;
	/* handling base color */
	base_alpha16 = ARGB32_ALPHA16(base);
	hue16 = rgb2hsv( ARGB32_RED16(base), ARGB32_GREEN16(base), ARGB32_BLUE16(base), &sat16, &val16 );
	cs->base_hue = hue162degrees(hue16);
	sat = val162percent(sat16);
	val = val162percent(val16);
	cs->base_sat = max(sat,ASCS_MIN_PRIMARY_SATURATION);
	cs->base_val = FIT_IN_RANGE(ASCS_MIN_PRIMARY_BRIGHTNESS, val, ASCS_MAX_PRIMARY_BRIGHTNESS);
	cs->base_argb = make_color_scheme_argb( base_alpha16, cs->base_hue, cs->base_sat, cs->base_val ) ;

	cs->inactive1_hue = normalize_degrees_val(cs->base_hue + angle) ;
	if( cs->inactive1_hue > ASCS_MIN_COLD_HUE && cs->inactive1_hue < ASCS_MAX_COLD_HUE &&
		cs->base_sat > ASCS_MIN_PRIMARY_SATURATION + ASCS_COLD_SATURATION_OFFSET )
		cs->inactive1_sat = cs->base_sat - ASCS_COLD_SATURATION_OFFSET ;
	else
		cs->inactive1_sat = cs->base_sat ;
	cs->inactive1_val = cs->base_val ;
	cs->inactive1_argb = make_color_scheme_argb( base_alpha16, cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val );

	cs->inactive2_hue = normalize_degrees_val(cs->base_hue - angle);
	if( cs->inactive2_hue > ASCS_MIN_COLD_HUE && cs->inactive2_hue < ASCS_MAX_COLD_HUE &&
		cs->base_sat > ASCS_MIN_PRIMARY_SATURATION + ASCS_COLD_SATURATION_OFFSET )
		cs->inactive2_sat = cs->base_sat - ASCS_COLD_SATURATION_OFFSET ;
	else
		cs->inactive2_sat = cs->base_sat ;
	cs->inactive2_val = cs->base_val ;
	cs->inactive2_argb = make_color_scheme_argb( base_alpha16, cs->inactive2_hue, cs->inactive2_sat, cs->inactive2_val );

	cs->active_hue = normalize_degrees_val(cs->base_hue - 180);
	cs->active_sat = cs->base_sat ;
	cs->active_val = cs->base_val ;
	cs->active_argb = make_color_scheme_argb( base_alpha16, cs->active_hue, cs->active_sat, cs->active_val );

	cs->inactive_text1_hue = normalize_degrees_val(cs->inactive1_hue - 180);
	cs->inactive_text1_sat = cs->base_sat ;
	cs->inactive_text1_val = cs->base_val ;
	if( is_light_hsv(cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val) )
		cs->inactive_text1_val = ASCS_BLACKING_BRIGHTNESS_LEVEL ;
	else
		cs->inactive_text1_sat = ASCS_WHITING_SATURATION_LEVEL ;
	cs->inactive_text1_argb = make_color_scheme_argb( base_alpha16, cs->inactive_text1_hue, cs->inactive_text1_sat, cs->inactive_text1_val );

	cs->inactive_text2_hue = normalize_degrees_val(cs->inactive2_hue - 180);
	cs->inactive_text2_sat = cs->base_sat ;
	cs->inactive_text2_val = cs->base_val ;
	if( is_light_hsv(cs->inactive2_hue, cs->inactive2_sat, cs->inactive2_val) )
		cs->inactive_text2_val = ASCS_BLACKING_BRIGHTNESS_LEVEL ;
	else
		cs->inactive_text2_sat = ASCS_WHITING_SATURATION_LEVEL ;
	cs->inactive_text2_argb = make_color_scheme_argb( base_alpha16, cs->inactive_text2_hue, cs->inactive_text2_sat, cs->inactive_text2_val );

	cs->active_text_sat = cs->base_sat ;
	cs->active_text_val = cs->base_val ;
	if( is_light_hsv(cs->base_hue, cs->base_sat, cs->base_val) )
		cs->active_text_val = ASCS_BLACKING_BRIGHTNESS_LEVEL ;
	else
		cs->active_text_sat = ASCS_WHITING_SATURATION_LEVEL ;
	cs->active_text_argb = make_color_scheme_argb( base_alpha16, cs->base_hue, cs->active_text_sat, cs->active_text_val );

	cs->high_inactive_argb = make_color_scheme_argb( base_alpha16, cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val + ASCS_HIGH_BRIGHTNESS_OFFSET);
	cs->high_active_argb   = make_color_scheme_argb( base_alpha16, cs->active_hue, cs->active_sat, cs->active_val + ASCS_HIGH_BRIGHTNESS_OFFSET);
	cs->disabled_text_argb = make_color_scheme_argb( base_alpha16, cs->inactive1_hue, ASCS_DISABLED_SATURATION_LEVEL, cs->inactive1_val);

	make_grad_argb( &(cs->base_grad[0]), base_alpha16, cs->base_hue, cs->base_sat, cs->base_val );
	make_grad_argb( &(cs->inactive1_grad[0]), base_alpha16, cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val );
	make_grad_argb( &(cs->inactive2_gradb[0]), base_alpha16, cs->inactive2_hue, cs->inactive2_sat, cs->inactive2_val );
	make_grad_argb( &(cs->active_grad[0]), base_alpha16, cs->active_hue, cs->active_sat, cs->active_val );
	make_grad_argb( &(cs->high_inactive_grad[0]), base_alpha16, cs->inactive1_hue, cs->inactive1_sat, cs->inactive1_val + ASCS_HIGH_BRIGHTNESS_OFFSET );
	make_grad_argb( &(cs->high_active_grad[0]), base_alpha16, cs->active_hue, cs->active_sat, cs->active_val + ASCS_HIGH_BRIGHTNESS_OFFSET );

	return cs;
}
