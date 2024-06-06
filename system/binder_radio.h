/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef BINDER_RADIO_H
#define BINDER_RADIO_H

#include <glib.h>
#include <glib-object.h>

#include "binder_client.h"

#define TYPE_BINDER_RADIO \
    (binder_radio_get_type ())
#define BINDER_RADIO(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_BINDER_RADIO, BinderRadio))
#define BINDER_RADIO_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_BINDER_RADIO, BinderRadioClass))
#define IS_BINDER_RADIO(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_BINDER_RADIO))
#define IS_BINDER_RADIO_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_BINDER_RADIO))
#define BINDER_RADIO_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_BINDER_RADIO, BinderRadioClass))

G_BEGIN_DECLS

typedef struct _BinderRadio BinderRadio;
typedef struct _BinderRadioClass BinderRadioClass;
typedef struct _BinderRadioPrivate BinderRadioPrivate;

struct _BinderRadio {
    BinderClient parent;
    BinderRadioPrivate *priv;
};

struct _BinderRadioClass {
    BinderClientClass parent_class;
};

GType           binder_radio_get_type            (void) G_GNUC_CONST;

GObject*        binder_radio_new                 (void);

G_END_DECLS

#endif

