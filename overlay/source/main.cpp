#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header

#include "splitter.hpp"
#include "dmntcht.h"



Thread t0;
Splitter* splitter = nullptr;
bool splitting_enabled = false;
bool thread_execute = true;


void updateSplitter(void*)
{
    while (thread_execute)
    {
        if (splitting_enabled)
        {
            if (splitter)
            {
                splitter->Update();
            }
        }
        svcSleepThread(1e8);
    }
}

class SplitterGui : public tsl::Gui {
public:
    SplitterGui() { }

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
        splitting_enabled = true;

        if (splitter)
        {
            time_string = splitter->GetSplitTime();
            split_string = splitter->GetSplitName();
        }
        else 
        {
            time_string = "0:00";
            split_string = "";
        }
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
                if (splitter)
                    splitter->Split();
                return true;
            }
            if (keysDown & HidNpadButton_Y)
            {
                if (splitter)
                    splitter->Undo();
                return true;
            }
            if (keysDown & HidNpadButton_X)
            {
                if (splitter)
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


class MainGui : public tsl::Gui {
public:
    MainGui() { }

    virtual tsl::elm::Element* createUI() override 
    {
        // 1. read in /switch/SplitNX/splitter.txt
        if (!splitter)
        {
            splitter = new Splitter("/switch/SplitNX/splitter.txt");
        }

        auto frame = new tsl::elm::OverlayFrame("SplitNx", "Tesla Overlay Version");
        auto list = new tsl::elm::List();

        // 3. Have button for connecting to LiveSplit
		connect_splitter = new tsl::elm::ListItem("Connect to Livesplit", "Not Connected");
		connect_splitter->setClickListener([&](uint64_t keys) {
			if (keys & HidNpadButton_A) {
                if (splitter)
                    splitter->Connect();
				return true;
			}
			return false;
		});
		list->addItem(connect_splitter);

        // 4. Have button for switching to next Gui
		auto split_mini = new tsl::elm::ListItem("Start Autosplitter");
		split_mini->setClickListener([&](uint64_t keys) {
			if (keys & HidNpadButton_A && (splitter && splitter->IsConnected())) {
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
			if (keys & HidNpadButton_A && splitter) {
                splitter->Reset();
				return true;
			}
			return false;
		});
		list->addItem(reset_button);

        // 2. Show settings on overlay
        if (splitter)
        {
            list->addItem(new tsl::elm::ListItem("IP", splitter->GetIp().c_str()));
            list->addItem(new tsl::elm::ListItem("Port", std::to_string(splitter->GetPort()).c_str()));
            list->addItem(new tsl::elm::ListItem("Number of splits", std::to_string(splitter->GetNumSplits()).c_str()));
        }

        frame->setContent(list);
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override 
    {
        splitting_enabled = false;
        if (splitter && splitter->IsConnected())
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

        threadCreate(&t0, updateSplitter, NULL, NULL, 0x400, 0x3F, -2);
        thread_execute = true;
        splitting_enabled = false;
        threadStart(&t0);
    }

    // Called at the end to clean up all services previously initialized
    virtual void exitServices() override 
    {
        socketExit();
        dmntchtExit();
        fsdevUnmountAll();

        thread_execute = false;
        threadWaitForExit(&t0);
        threadClose(&t0);
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