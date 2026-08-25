#include <obs-module.h>
#include <obs-frontend-api.h>

#include "dashboard-widget.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("vmix-dashboard-core", "en-US")

static DashboardWidget *dashboard = nullptr;

bool obs_module_load(void)
{
    dashboard = new DashboardWidget();
    const bool added = obs_frontend_add_dock_by_id(
        "vmix-dashboard-core", "vMix Dashboard", dashboard);

    if (!added) {
        blog(LOG_ERROR, "[vMix Dashboard Core] Failed to register dock");
        delete dashboard;
        dashboard = nullptr;
        return false;
    }

    blog(LOG_INFO, "[vMix Dashboard Core] Loaded");
    return true;
}

void obs_module_unload(void)
{
    dashboard = nullptr; // OBS owns the registered dock widget.
    blog(LOG_INFO, "[vMix Dashboard Core] Unloaded");
}

const char *obs_module_name(void)
{
    return "vMix Dashboard Core";
}

const char *obs_module_description(void)
{
    return "Scene thumbnail dashboard with independent drag-and-drop ordering.";
}

const char *obs_module_author(void)
{
    return "OpenAI / user-requested implementation";
}
