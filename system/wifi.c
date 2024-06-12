/*
 * Copyright Cedric Bellegarde <cedric.bellegarde@adishatz.org>
 */

#include <stdio.h>
#include <stdarg.h>
#include <net/if.h>
#include <stdint.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>

#include <gio/gio.h>

#include "wifi.h"
#include "../common/utils.h"

struct _WiFiPrivate {
    struct nl_sock *socket;

    gint ifindex;
    gint genl_family_id;
};

G_DEFINE_TYPE_WITH_CODE (
    WiFi,
    wifi,
    G_TYPE_OBJECT,
    G_ADD_PRIVATE (WiFi)
)

static int finish_handler (struct nl_msg *msg,
                           void          *arg)
{
    int *ret = arg;
    *ret = 0;
    return NL_SKIP;
}

/* Yes, we only handle one WiFi interface */
static int get_wifi_interface (struct nl_msg *msg,
                               void          *arg)
{
    WiFi *self = WIFI (arg);
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data (nlmsg_hdr (msg));

    g_return_val_if_fail (self->priv->ifindex == -1, NL_SKIP);

    nla_parse(
        tb_msg,
        NL80211_ATTR_MAX,
        genlmsg_attrdata (gnlh, 0),
        genlmsg_attrlen(gnlh, 0),
        NULL
    );

    if (tb_msg[NL80211_ATTR_IFNAME]) {
        self->priv->ifindex = if_nametoindex (
            nla_get_string(tb_msg[NL80211_ATTR_IFNAME])
        );
    }

    return NL_SKIP;
}

static struct nl_msg *
nl80211_alloc_msg(WiFi    *self,
                  uint8_t  cmd)
{
    struct nl_msg *msg = NULL;

    msg = nlmsg_alloc();

    g_return_val_if_fail (msg != NULL, NULL);

    genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, self->priv->genl_family_id, 0, 0, cmd, 0);
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, self->priv->ifindex);
    return msg;

nla_put_failure:
    g_return_val_if_reached(NULL);
}

static void
wifi_dispose (GObject *wifi)
{
    G_OBJECT_CLASS (wifi_parent_class)->dispose (wifi);
}

static void
wifi_finalize (GObject *wifi)
{
    WiFi *self = WIFI (wifi);

    nl_socket_free (self->priv->socket);

    G_OBJECT_CLASS (wifi_parent_class)->finalize (wifi);
}

static void
wifi_class_init (WiFiClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = wifi_dispose;
    object_class->finalize = wifi_finalize;
}

static void
wifi_init (WiFi *self)
{
    struct nl_msg *msg;
    struct nl_cb *cb;
    gint err = 1;

    self->priv = wifi_get_instance_private (self);
    self->priv->ifindex = -1;

    self->priv->socket = nl_socket_alloc ();

    g_return_if_fail (self->priv->socket != NULL);

    nl_socket_set_buffer_size(self->priv->socket, 8192, 8192);

    if (genl_connect (self->priv->socket)) {
        nl_socket_free (self->priv->socket);
        return;
    }

    self->priv->genl_family_id = genl_ctrl_resolve(
        self->priv->socket, "nl80211"
    );

    msg = nlmsg_alloc();
    g_return_if_fail (msg != NULL);

    cb = nl_cb_alloc (NL_CB_DEFAULT);
    if (cb == NULL) {
        nlmsg_free (msg);
        return;
    }

    nl_cb_set (cb, NL_CB_VALID, NL_CB_CUSTOM, get_wifi_interface, self);
    genlmsg_put (msg, 0, 0, self->priv->genl_family_id , 0,
                NLM_F_DUMP, NL80211_CMD_GET_INTERFACE, 0);
    nl_send_auto_complete(self->priv->socket, msg);

    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);

    while (err > 0)
        nl_recvmsgs(self->priv->socket, cb);

    nlmsg_free (msg);
    nl_cb_put (cb);
}

/**
 * wifi_new:
 *
 * Creates a new #WiFi
 *
 * Returns: (transfer full): a new #WiFi
 *
 **/
GObject *
wifi_new (void)
{
    GObject *wifi;

    wifi = g_object_new (TYPE_WIFI, NULL);

    return wifi;
}

/**
 * wifi_set_powersave:
 *
 * Set wifi devices to powersave
 *
 * @param #WiFi
 * @param powersave: True to enable powersave
 */
void
wifi_set_powersave (WiFi     *self,
                    gboolean  powersave)
{
    struct nl_msg *msg  = NULL;

    g_return_if_fail (self->priv->socket != NULL);
    g_return_if_fail (self->priv->ifindex != -1);

    msg = nl80211_alloc_msg(self, NL80211_CMD_SET_POWER_SAVE);
    nla_put_u32(msg,
                NL80211_ATTR_PS_STATE,
                powersave == 1 ? NL80211_PS_ENABLED : NL80211_PS_DISABLED);
    nl_send_auto(self->priv->socket, msg);

    nlmsg_free(msg);
}