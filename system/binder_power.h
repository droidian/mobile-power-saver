/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#ifndef BINDER_POWER_H
#define BINDER_POWER_H

#include <glib.h>
#include <glib-object.h>

#include "binder_client.h"

#define TYPE_BINDER_POWER \
    (binder_power_get_type ())
#define BINDER_POWER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST \
    ((obj), TYPE_BINDER_POWER, BinderPower))
#define BINDER_POWER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_CAST \
    ((cls), TYPE_BINDER_POWER, BinderPowerClass))
#define IS_BINDER_POWER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE \
    ((obj), TYPE_BINDER_POWER))
#define IS_BINDER_POWER_CLASS(cls) \
    (G_TYPE_CHECK_CLASS_TYPE \
    ((cls), TYPE_BINDER_POWER))
#define BINDER_POWER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS \
    ((obj), TYPE_BINDER_POWER, BinderPowerClass))

G_BEGIN_DECLS

typedef struct _BinderPower BinderPower;
typedef struct _BinderPowerClass BinderPowerClass;
typedef struct _BinderPowerPrivate BinderPowerPrivate;

struct _BinderPower {
    BinderClient parent;
    BinderPowerPrivate *priv;
};

struct _BinderPowerClass {
    BinderClientClass parent_class;
};

GType           binder_power_get_type            (void) G_GNUC_CONST;

GObject*        binder_power_new                 (void);

G_END_DECLS

#endif

