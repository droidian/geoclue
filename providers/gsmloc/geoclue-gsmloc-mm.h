/*
 * Geoclue
 * geoclue-gsmloc-mm.h - An Address/Position provider for ModemManager
 *
 * Author: Dan Williams <dcbw@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef _GEOCLUE_GSMLOC_MM
#define _GEOCLUE_GSMLOC_MM

#include <glib-object.h>

G_BEGIN_DECLS

#define GEOCLUE_TYPE_GSMLOC_MM (geoclue_gsmloc_mm_get_type ())

#define GEOCLUE_GSMLOC_MM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEOCLUE_TYPE_GSMLOC_MM, GeoclueGsmlocMm))
#define GEOCLUE_GSMLOC_MM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEOCLUE_TYPE_GSMLOC_MM, GeoclueGsmlocMmClass))
#define GEOCLUE_IS_GSMLOC_MM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEOCLUE_TYPE_GSMLOC_MM))
#define GEOCLUE_IS_GSMLOC_MM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEOCLUE_TYPE_GSMLOC_MM))

typedef struct _GeoclueGsmlocMm {
	GObject parent;
} GeoclueGsmlocMm;

typedef struct _GeoclueGsmlocMmClass {
	GObjectClass parent_class;

	void (*network_data_changed) (GeoclueGsmlocMm *mm,
	                              char *mcc, char *mnc,
	                              char *lac, char *cid);
} GeoclueGsmlocMmClass;

GType geoclue_gsmloc_mm_get_type (void);

GeoclueGsmlocMm *geoclue_gsmloc_mm_new (void);

G_END_DECLS

#endif  /* _GEOCLUE_GSMLOC_MM */

