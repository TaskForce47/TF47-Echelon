#include "script_component.hpp"

class CfgPatches {
    class tf47_intercept_template_main {
        name = "TF47 Intercept Template";
        units[] = {};
        weapons[] = {};
        requiredVersion = 2.02;
        requiredAddons[] = {"intercept_core"};
        authors[] = { "Willard, Dragon" };
        url = "https://github.com/taskforce47/tf47_intercept_template";
        VERSION_CONFIG;
    };
};
class Intercept {
    class tf47 {
        class tf47_intercept_template {
            pluginName = "tf47_intercept_template";
        };
    };
};
