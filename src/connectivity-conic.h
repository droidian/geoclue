/*
 * Geoclue
 * connectivity-conic.h
 *
 * Author: Jussi Kukkonen <jku@o-hand.com>
 * Copyright 2007 by Garmin Ltd. or its subsidiaries
 */

#ifndef _CONNECTIVITY_CONIC_H
#define _CONNECTIVITY_CONIC_H

#include <glib-object.h>
#include <conicconnection.h>
#include "connectivity.h"

G_BEGIN_DECLS

#define GEOCLUE_TYPE_CONIC (geoclue_conic_get_type ())
#define GEOCLUE_CONIC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_CONIC, GeoclueConic))

typedef struct {
	GObject parent;
	
	/* private */
	GeoclueNetworkStatus status;
	ConIcConnection *conic;
} GeoclueConic;

typedef struct {
	GObjectClass parent_class;
} GeoclueConicClass;

GType geoclue_conic_get_type (void);

G_END_DECLS

#endif
