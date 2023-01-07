#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header
#include <json.hpp>
#include <fstream>
#include <dirent.h>

#include "splitter.hpp"
#include "dmntcht.h"


using json = nlohmann::json;
bool run_threads = true;

void updateSplitter(void* arg)
{
    Splitter* splitter = (Splitter*) arg;
    while (run_threads)
    {
        if (splitter)
            splitter->Update();
        svcSleepThread(1e8);
    }
}

class SplitterGui : public tsl::Gui {
public:
    SplitterGui(Splitter &splitter) :
        splitter(splitter)
    { 
    }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
		auto rootFrame = new tsl::elm::CustomDrawer([&](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
			renderer->clearScreen();
            if (draw_square)
            {
                renderer->drawRect(0, 0, 125, 20, a(tsl::style::color::ColorFrameBackground));
                if (time_string != "0:00")
                {
                    renderer->drawRect(0, 0, 125, 35, a(tsl::style::color::ColorFrameBackground));
                    renderer->drawString(split_string.c_str(), false, 0, 30, 15, renderer->a(tsl::Color(255,255,255,255)));
                }
                renderer->drawString(time_string.c_str(), false, 0, 15, 15, renderer->a(tsl::Color(255,255,255,255)));
            }
		});

        rootFrame->setBoundaries(0, 0, tsl::cfg::FramebufferWidth, tsl::cfg::FramebufferHeight);

		return rootFrame;
    }

    // Called once every frame to update values
    virtual void update() override {
        tsl::hlp::requestForeground(false);
        time_string = splitter.GetSplitTime();
        split_string = splitter.GetSplitName();
    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override 
    {
        if ((keysHeld & HidNpadButton_R) && (keysHeld & HidNpadButton_ZR) && !(keysHeld & HidNpadButton_L) && !(keysHeld & HidNpadButton_ZL))
        {
            if (keysDown & HidNpadButton_Minus)
            {
                draw_square = false;
                splitter.EnableSplitting(false);
                tsl::hlp::requestForeground(true);
                tsl::goBack();
                return true;
            }
            if (keysDown & HidNpadButton_A)
            {
                splitter.Split();
                return true;
            }
            if (keysDown & HidNpadButton_Y)
            {
                splitter.Undo();
                return true;
            }
            if (keysDown & HidNpadButton_X)
            {
                splitter.Skip();
                return true;
            }
            if (keysDown & HidNpadButton_Plus)
            {
                draw_square = !draw_square;
            }
        }

        // Returning true prevents Tesla from returning to previous menu on B press
        return true;   // Return true here to singal the inputs have been consumed
    }

private:
    bool draw_square = false;
    Splitter& splitter;
    std::string time_string = "0:00";
    std::string split_string = "";
};

/*
class IpSelect : public tsl::Gui {
public:
    IpSelect() { }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
		auto rootFrame = new tsl::elm::OverlayFrame("", "");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Create and add a new list item to the list
        list->addItem(new tsl::elm::ListItem("Default List Item"));

		rootFrame->setContent(list);

		return rootFrame;
    }

    // Called once every frame to update values
    virtual void update() override {

    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override 
    {
        if ((keysHeld & HidNpadButton_R) && (keysHeld & HidNpadButton_ZR) && (keysDown & HidNpadButton_Minus)) {
            tsl::hlp::requestForeground(true);
			tsl::goBack();
			return true;
		}
        // Returning true prevents Tesla from returning to previous menu on B press
        return true;   // Return true here to singal the inputs have been consumed
    }
};
*/

class SplitSelect : public tsl::Gui {
private:
    Splitter& splitter;
    std::vector<Splits> splits;
public:
    SplitSelect(Splitter& splitter, std::vector<Splits> splits) :
        splitter(splitter), splits(splits)
    { }

    virtual tsl::elm::Element* createUI() override {
		auto rootFrame = new tsl::elm::OverlayFrame("Select splits to use", " ");

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // Create and add a new list item to the list
        for (auto s : splits)
        {
            auto i = new tsl::elm::ListItem(s.game.c_str(), s.category.c_str());
            i->setClickListener([this, s](uint64_t keys) {
                if (keys & HidNpadButton_A)
                {
                    this->splitter.Reload(s);
                    tsl::goBack();
                    return true;
                }
                return false;
            });
            list->addItem(i);
        }
        // list->addItem(new tsl::elm::ListItem("Default List Item"));

		rootFrame->setContent(list);

		return rootFrame;
    }

    // Called once every frame to update values
    virtual void update() override {

    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override 
    {
        if (false) {
			return true;
		}
        
        return false;
    }
};

class MainGui : public tsl::Gui {
private:
    Splitter splitter;
    std::vector<Splits> splits;
    std::string ip;
    int port;
    Thread t_splitter;
    tsl::hlp::ScopeGuard splitterGuard;

    void loadConfigs()
    {
        json configJson;
        std::string default_config;
        // Load main config file for IP, port, etc
        std::ifstream configFile("sdmc:/switch/SplitNX/config.json");
        if (!configFile.fail()) {
            try {
                configFile >> configJson;
                ip = configJson["ip"].get<std::string>();
                port = configJson["port"].get<int>();
                default_config = configJson["default_splits"].get<std::string>();
            } catch (json::parse_error &e) {}
        }

        // Load splits config files
        bool default_loaded = false;
        DIR *switchnxConfDir = opendir("sdmc:/switch/SplitNX/");
        if (switchnxConfDir != nullptr) {
            struct dirent *ent;
            while ((ent = readdir(switchnxConfDir)) != nullptr) {
                // Don't read config again
                if (strcmp(ent->d_name, "config.json") == 0) continue;

                // Make sure file is a json
                if (std::string(ent->d_name).substr(strlen(ent->d_name) - 5) != ".json") continue;

                // Parse
                std::ifstream splitsConfig("sdmc:/switch/SplitNX/" + std::string(ent->d_name));
                if (!splitsConfig.fail()) {
                    try {
                        json newSplits;
                        splitsConfig >> newSplits;
                        newSplits["file"] = std::string(ent->d_name);
                        splits.push_back(Splits(newSplits));

                        if (std::string(ent->d_name) == default_config)
                        {
                            splitter.Reload(splits.back());
                            default_loaded = true;
                        }
                    } catch (json::parse_error &e) {}
                }
            }
        }
        closedir(switchnxConfDir);

        if (!default_loaded && splits.size() > 0) splitter.Reload(splits.front());
    }

public:
    MainGui() :
        splitterGuard([&] { run_threads = false; threadWaitForExit(&t_splitter); threadClose(&t_splitter); })
    {
        loadConfigs();

        run_threads = true;
        threadCreate(&t_splitter, updateSplitter, (void*)&this->splitter, NULL, 0x800, 0x3F, -2);
        threadStart(&t_splitter);
    }

    virtual tsl::elm::Element* createUI() override 
    {
        auto frame = new tsl::elm::OverlayFrame("SplitNX", "Tesla Overlay Version");
        auto list = new tsl::elm::List();

        // 3. Have button for connecting to LiveSplit
		auto connect_button = new tsl::elm::ListItem("Connect to Livesplit", "Not Connected");
		connect_button->setClickListener([this, connect_button](uint64_t keys) {
			if (keys & HidNpadButton_A) {
                splitter.Connect(ip, port);
                return true;
            }

			return false;
		});
		list->addItem(connect_button);

        // 4. Have button for switching to next Gui
		auto start_button = new tsl::elm::ListItem("Start Autosplitter");
		start_button->setClickListener([&](uint64_t keys) {
			if (keys & HidNpadButton_A && splitter.IsConnected()) {
                tsl::hlp::requestForeground(false);
                splitter.EnableSplitting(true);
				tsl::changeTo<SplitterGui>(splitter);
				return true;
			}
			return false;
		});
		list->addItem(start_button);

        // 5. Have button for resetting splits
		auto reset_button = new tsl::elm::ListItem("Reset splits");
		reset_button->setClickListener([&](uint64_t keys) {
			if (keys & HidNpadButton_A) {
                splitter.Reset();
				return true;
			}
			return false;
		});
		list->addItem(reset_button);

        auto select_button = new tsl::elm::ListItem("Select Splits", splitter.GetSplits().game + "\n" + splitter.GetSplits().category);
        select_button->setClickListener([&](uint64_t keys) {
            if (keys & HidNpadButton_A)
            {
                tsl::changeTo<SplitSelect>(splitter, splits);
                return true;
            }
            return false;
        });
        list->addItem(select_button);

        // 2. Show settings on overlay
        try
        {
            list->addItem(new tsl::elm::ListItem("IP", ip.c_str()));
            list->addItem(new tsl::elm::ListItem("Port", std::to_string(port).c_str()));
        } catch (json::parse_error &e) { }
        

        // list will always have click listener running, use it to update connected text
        list->setClickListener([this, connect_button, select_button](uint64_t keys) {
            if (this->splitter.IsConnected())
                connect_button->setValue("Connected");
            else
                connect_button->setValue("Not Connected");

            select_button->setValue(splitter.GetSplits().game + "\n" + splitter.GetSplits().category);
			return false;
		});

        frame->setContent(list);
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override { }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override 
    {
        return false;   // Return true here to singal the inputs have been consumed
    }
};

class SplitterOverlay : public tsl::Overlay {
public:
    // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    // Called at the start to initialize all services necessary for this Overlay
    virtual void initServices() override
    {
        fsdevMountSdmc();
        dmntchtInitialize();
        socketInitializeDefault();
    }

    // Called at the end to clean up all services previously initialized
    virtual void exitServices() override 
    {
        socketExit();
        dmntchtExit();
        fsdevUnmountAll();
    }

    // Called before overlay wants to change from invisible to visible state
    virtual void onShow() override 
    {

    }

    // Called before overlay wants to change from visible to invisible state
    virtual void onHide() override 
    {

    }

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override 
    {
        return initially<MainGui>();  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
    }
};

int main(int argc, char **argv) {
    return tsl::loop<SplitterOverlay>(argc, argv);
}