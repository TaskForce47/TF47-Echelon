#include "script_component.hpp"

class CfgPatches {
    class tf47_echelon_main {
        name = "TF47 Intercept Echelon";
        units[] = {};
        weapons[] = {};
        requiredVersion = 2.02;
        requiredAddons[] = {"intercept_core"};
        authors[] = { "Willard, Dragon" };
        url = "https://github.com/taskforce47/TF47-Echelon";
        VERSION_CONFIG;
    };
};
class Intercept {
    class tf47 {
        class tf47_echelon {
            pluginName = "tf47_echelon";
        };
    };
};
