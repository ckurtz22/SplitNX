#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

#include "splitter.hpp"
#include "dmntcht.h"


Splitter* splitter = nullptr;
char debug_text[64] = "";


class SplitterGui : public tsl::Gui {
public:
    SplitterGui() { }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
		auto rootFrame = new tsl::elm::OverlayFrame("", "");

		auto Status = new tsl::elm::CustomDrawer([&](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
			renderer->clearScreen();
            if (draw_square)
            {
                // renderer->drawString("Press ZR+R+Minus to exit", false, 0, 15, 15, renderer->a(tsl::Color(0,0,0,255)));
                std::string time = splitter->GetSplitTime();
                renderer->drawRect(0, 0, 125, 20, a(tsl::style::color::ColorFrameBackground));
                if (time != "0:00")
                {
                    renderer->drawRect(0, 0, 125, 35, a(tsl::style::color::ColorFrameBackground));
                    renderer->drawString(splitter->GetSplitName().c_str(), false, 0, 30, 15, renderer->a(tsl::Color(255,255,255,255)));
                }
                renderer->drawString(time.c_str(), false, 0, 15, 15, renderer->a(tsl::Color(255,255,255,255)));
            }
		});

		rootFrame->setContent(Status);

		return rootFrame;
    }

    // Called once every frame to update values
    virtual void update() override {
        tsl::hlp::requestForeground(false);
        splitter->Update();
        snprintf(debug_text, sizeof(debug_text), "%s", splitter->GetSplitTime().c_str());
    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override 
    {
        if ((keysHeld & HidNpadButton_R) && (keysHeld & HidNpadButton_ZR) && !(keysHeld & HidNpadButton_L) && !(keysHeld & HidNpadButton_ZL))
        {
            if (keysDown & HidNpadButton_Minus)
            {
                draw_square = false;
                tsl::hlp::requestForeground(true);
                tsl::goBack();
                return true;
            }
            if (keysDown & HidNpadButton_A)
            {
                splitter->Split();
                return true;
            }
            if (keysDown & HidNpadButton_Y)
            {
                splitter->Undo();
                return true;
            }
            if (keysDown & HidNpadButton_X)
            {
                splitter->Skip();
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
    bool draw_square = true;
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


class MainGui : public tsl::Gui {
public:
    MainGui() { }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override 
    {
        // 1. read in /switch/SplitNX/splitter.txt
        if (splitter == nullptr)
        {
            splitter = new Splitter("/switch/SplitNX/splitter.txt");
        }

        auto frame = new tsl::elm::OverlayFrame("SplitNx", "Tesla Overlay Version");
        auto list = new tsl::elm::List();

        // 3. Have button for connecting to LiveSplit
		connect_splitter = new tsl::elm::ListItem("Connect to Livesplit", "Not Connected");
		connect_splitter->setClickListener([&](uint64_t keys) {
			if (keys & HidNpadButton_A) {
                splitter->Connect();
				return true;
			}
			return false;
		});
		list->addItem(connect_splitter);

        // 4. Have button for switching to next Gui
		auto split_mini = new tsl::elm::ListItem("Start Autosplitter");
		split_mini->setClickListener([&](uint64_t keys) {
			if (splitter->IsConnected() && keys & HidNpadButton_A) {
                tsl::hlp::requestForeground(false);
				tsl::changeTo<SplitterGui>();
				return true;
			}
			return false;
		});
		list->addItem(split_mini);

        // 5. Have button for resetting splits
		auto reset_button = new tsl::elm::ListItem("Reset splits");
		reset_button->setClickListener([&](uint64_t keys) {
			if (splitter->IsConnected() && keys & HidNpadButton_A) {
                splitter->Reset();
				return true;
			}
			return false;
		});
		list->addItem(reset_button);

        // 2. Show settings on overlay
        list->addItem(new tsl::elm::ListItem("IP", splitter->GetIp().c_str()));
        list->addItem(new tsl::elm::ListItem("Port", std::to_string(splitter->GetPort()).c_str()));
        list->addItem(new tsl::elm::ListItem("Number of splits", std::to_string(splitter->GetNumSplits()).c_str()));


        frame->setContent(list);
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override 
    {
        if (splitter->IsConnected())
            connect_splitter->setValue("Connected");
        else
            connect_splitter->setValue("Not Connected");

    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override 
    {
        return false;   // Return true here to singal the inputs have been consumed
    }

private:
    tsl::elm::ListItem* connect_splitter;
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