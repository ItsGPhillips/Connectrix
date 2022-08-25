
#include "left_sidebar.h"
#include "../../settings.h"

#include "data.h"
#include "defs.h"
#include <memory>
#include <ranges>
#include <stdlib.h>
#include <variant>

namespace cntx::node_graph
{
   using juce::FileSearchPath;

   class PluginListBoxModel: public juce::ListBoxModel
   {
   public:
      PluginListBoxModel (juce::Array<juce::PluginDescription>& descriptions)
         : m_descriptions (descriptions)
      {}
      int getNumRows () override { return m_descriptions.size (); }
      void paintListBoxItem (int rowNumber,
                             juce::Graphics& g,
                             int width,
                             int height,
                             bool rowIsSelected) override
      {
         g.fillAll (juce::Colours::black);
         g.setColour (juce::Colours::white);
         g.drawFittedText (
            m_descriptions[rowNumber].name, { 0, 0, width, height }, juce::Justification::left, 1);
      }
      juce::var getDragSourceDescription (const juce::SparseSet<int>& selectedRows) override
      {
         jassert (selectedRows.size () == 1);
         return selectedRows[0];
      }

   private:
      juce::Array<juce::PluginDescription>& m_descriptions;
   };

   struct PluginList: public juce::ListBox,
                      public juce::ChangeListener
   {
      PluginList ()
         : juce::ListBox ("Test", new PluginListBoxModel (m_descriptions)),
           m_descriptions (GlobalStateManager::getPluginManager ().getKnownPluginList ().getTypes ())
      {
         setName ("Plugins");
         GlobalStateManager::getPluginManager ().getKnownPluginList ().addChangeListener (this);
         updateContent ();
      }

      ~PluginList ()
      {
         GlobalStateManager::getPluginManager ().getKnownPluginList ().removeChangeListener (this);
      }

      void changeListenerCallback (juce::ChangeBroadcaster* source) override
      {
         if (source == &GlobalStateManager::getPluginManager ().getKnownPluginList ()) {
            m_descriptions =
               GlobalStateManager::getPluginManager ().getKnownPluginList ().getTypes ();
         }
         updateContent ();
      }

   private:
      juce::Array<juce::PluginDescription> m_descriptions;
   };

   class PluginTreeViewItem: public juce::TreeViewItem
   {
   public:
      PluginTreeViewItem (Box<juce::KnownPluginList::PluginTree> plugin_tree)
         : m_node (std::move (plugin_tree))
      {}
      PluginTreeViewItem (juce::PluginDescription desc) : m_node (std::move (desc)) {}

      void paintItem (juce::Graphics& g, i32 width, i32 height) override
      {
         using namespace juce;
         std::visit (overloaded {
                        [&] (const Box<juce::KnownPluginList::PluginTree>&) {
                           g.fillAll (Colours::black);
                           g.setColour (Colours::white);
                           if (isSelected () || isOpen ()) {
                              // g.setFont (15.0f);
                              g.setFont (Font (15.0f, Font::FontStyleFlags::bold));
                           } else {
                              g.setFont (Font (15.0f, Font::FontStyleFlags::plain));
                           }
                        },
                        [&] (const juce::PluginDescription&) {
                           if (isSelected ()) {
                              g.fillAll (Colours::cyan);
                              g.setColour (Colours::black);
                              // g.setFont (15.0f);
                           } else {
                              g.fillAll (Colours::black);
                              g.setColour (Colours::white);
                           }
                           g.setFont (Font (15.0f, Font::FontStyleFlags::plain));
                        },
                     },
                     m_node);
         g.drawText (getUniqueName (), 4, 0, width - 4, height, Justification::centredLeft, true);
      }

      void paintOpenCloseButton (juce::Graphics& g,
                                 const juce::Rectangle<float>& area,
                                 juce::Colour backgroundColour,
                                 bool isMouseOver) override
      {
         using namespace juce;
         juce::Path icon = [&] {
            auto path = juce::Path ();
            auto size = std::min (area.getWidth (), area.getHeight ());
            Rectangle<float> bounds = { size, size };
            bounds.reduce (5, 5);
            path.addEllipse (bounds);
            return path;
         }();

         if (isOpen ()) {
            g.setColour (Colours::cyan);
         } else {
            auto colour = Colours::grey;
            if (isMouseOver) {
               colour = colour.brighter ();
            }
            g.setColour (colour);
         }

         g.fillPath (icon);
      }

      [[nodiscard]] bool canBeSelected () const override { return true; }

      void itemOpennessChanged (bool isNowOpen) override
      {
         if (isNowOpen && getNumSubItems () == 0) {
            refreshSubItems ();
         } else {
            clearSubItems ();
         }
      }

      [[nodiscard]] juce::String getUniqueName () const override
      {
         return std::visit (
            overloaded {
               [] (const Box<juce::KnownPluginList::PluginTree>& tree) { return tree->folder; },
               [] (const juce::PluginDescription& desc) { return desc.name; },
            },
            m_node);
      }
      bool mightContainSubItems () override
      {
         return std::visit (overloaded {
                               [] (const Box<juce::KnownPluginList::PluginTree>&) { return true; },
                               [] (const juce::PluginDescription&) { return false; },
                            },
                            m_node);
      }

      juce::var getDragSourceDescription () override
      {
         return std::visit (
            overloaded {
               [] (const Box<juce::KnownPluginList::PluginTree>&) { return juce::var (); },
               [] (const juce::PluginDescription& desc) {
                  return juce::var (desc.createIdentifierString ());
               },
            },
            m_node);
      }

   private:
      void refreshSubItems ()
      {
         clearSubItems ();
         std::visit (overloaded {
                        [&] (const Box<juce::KnownPluginList::PluginTree>& tree) {
                           while (!tree->subFolders.isEmpty ()) {
                              auto* sub_folder = tree->subFolders.getLast ();
                              addSubItem (new PluginTreeViewItem (
                                 Box<juce::KnownPluginList::PluginTree> (sub_folder)));
                              tree->subFolders.removeLast (1, false);
                           }
                           for (auto& desc : tree->plugins) {
                              addSubItem (new PluginTreeViewItem (desc));
                           }
                        },
                        [&] (const juce::PluginDescription&) {},
                     },
                     m_node);
      }

      std::variant<Box<juce::KnownPluginList::PluginTree>, juce::PluginDescription> m_node;
   };

   class PluginTreeView: public juce::Component,
                         juce::ChangeListener
   {
   public:
      PluginTreeView ()
      {
         GlobalStateManager::getPluginManager ().getKnownPluginList ().addChangeListener (this);
         m_tree.setRootItemVisible (false);
         m_tree.setIndentSize (18);
         update ();
         addAndMakeVisible (m_tree);
         setName ("Plugin Tree");
      }

      ~PluginTreeView () override
      {
         GlobalStateManager::getPluginManager ().getKnownPluginList ().removeChangeListener (this);
         m_tree.setRootItem (nullptr);
      }

      void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }

      void resized () override { m_tree.setBounds (getLocalBounds ()); }

      void stringify_plugin_tree_rec (i32 depth,
                                      juce::String& output,
                                      juce::KnownPluginList::PluginTree& tree)
      {
         const auto* const indent_chars = "  ";
         juce::String indent;
         for (auto i = depth; --depth > 0;) {
            juce::ignoreUnused (i);
            indent << indent_chars;
         }
         output << indent << "[" << tree.folder << "]\n";
         indent << indent_chars;
         // for (auto desc : tree.plugins) {
         //    output << indent << " - " << desc.name << "\n";
         // }
         depth++;
         for (auto* sub_folder : tree.subFolders) {
            stringify_plugin_tree_rec (depth, output, *sub_folder);
         }
      }

      void update ()
      {
         auto& plugin_list = GlobalStateManager::getPluginManager ().getKnownPluginList ();
         // plugin_list.sort (juce::KnownPluginList::SortMethod::sortByManufacturer, true);
         auto tree = juce::KnownPluginList::createTree (
            plugin_list.getTypes (), juce::KnownPluginList::SortMethod::sortByManufacturer);

         juce::String output;
         stringify_plugin_tree_rec (0, output, *tree);

         LOG_DEBUG ("PluginTree\n{}", output);

         m_tree.setRootItem (nullptr);
         m_root = std::make_unique<PluginTreeViewItem> (std::move (tree));
         m_tree.setRootItem (m_root.get ());
      }

      void changeListenerCallback (juce::ChangeBroadcaster* source) override
      {
         if (source == &GlobalStateManager::getPluginManager ().getKnownPluginList ()) {
            update ();
         }
      }

   private:
      juce::TreeView m_tree;
      Box<PluginTreeViewItem> m_root;
   };

   //=========================================================================================
   //=========================================================================================

   struct TestComponent: public juce::Component
   {
   public:
      TestComponent (juce::String text) { setName (text); }
      void paint (juce::Graphics& g) override
      {
         using namespace juce;
         g.fillAll (Colours::ivory);
         g.drawFittedText (getName (), getLocalBounds (), Justification::centred, 1);
      }
      void resized () override {}

   private:
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestComponent)
   };

   class SidebarButton: public juce::Button
   {
   public:
      SidebarButton (ButtonBar& parentButtonBar, juce::String text, juce::Path iconPath)
         : juce::Button (text), m_iconPath (iconPath), m_parentButtonBar (parentButtonBar)
      {
         setToggleState (false, juce::NotificationType::dontSendNotification);
         setClickingTogglesState (true);
         setTooltip (text);
         setMouseCursor (juce::MouseCursor::PointingHandCursor);
         setPaintingIsUnclipped (true);

         onClick = [this] {
            if (getToggleState ()) {
               MessageQueue<SidebarMessage>::getInstance ()->postMessage (OpenSidePanel {
                  .index = m_parentButtonBar.getButtonIndex (this),
               });
               m_parentButtonBar.setActiveButton (this);
            } else {
               m_parentButtonBar.setActiveButton (nullptr);
            }
         };
      }

      //==============================================================================
      void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool) override
      {
         using namespace juce;
         if (shouldDrawButtonAsHighlighted || getToggleState ()) {
            g.setColour (Colour::fromRGB (153, 211, 224));
         } else {
            g.setColour (Colours::grey.darker (1.0));
         }

         auto iconBounds = getLocalBounds ().reduced (12).toFloat ();
         m_iconPath.scaleToFit (iconBounds.getX (),
                                iconBounds.getY (),
                                iconBounds.getWidth (),
                                iconBounds.getHeight (),
                                true);

         g.fillPath (m_iconPath);

         if (getToggleState ()) {
            g.setColour (Colour::fromRGB (153, 211, 224));
            auto bounds = getLocalBounds ();
            Path highlightPath;
            highlightPath.addRectangle (bounds.removeFromLeft (2));
            g.fillPath (highlightPath);
         }
      }

   private:
      juce::Path m_iconPath;
      ButtonBar& m_parentButtonBar;
   };
   // SettingsPage ==================================================================

   juce::FileSearchPath getPluginSearchPaths ()
   {
      if (auto* props = GlobalStateManager::getAppProperties ().getCommonSettings (true)) {
         if (auto search_paths = props->getXmlValue ("VST3_SEARCH_PATHS")) {
            jassert (search_paths->hasAttribute ("path"));
            return search_paths->getStringAttribute ("paths");
         }
         auto default_paths = juce::VST3PluginFormat ().getDefaultLocationsToSearch ();
         props->setValue ("VST3_SEARCH_PATHS", default_paths.toString ());
         props->saveIfNeeded ();
         return default_paths;
      }
      return {};
   }

   class SettingsPage;

   class Scanner: private juce::Timer
   {
   public:
      Scanner (SettingsPage& parent, const juce::String& title, const juce::String& text)
         : m_parent (parent), m_format (new juce::VST3PluginFormat ())
      {
         pathChooserWindow = std::make_unique<juce::AlertWindow> (
            TRANS ("Select folders to scan..."), juce::String (), juce::MessageBoxIconType::NoIcon);
         progressWindow = std::make_unique<juce::AlertWindow> (
            title, text, juce::MessageBoxIconType::NoIcon);

         auto& plugin_manager = cntx::GlobalStateManager::getPluginManager ();

         const auto blacklisted = plugin_manager.getKnownPluginList ()
                                     .getBlacklistedFiles (); // owner.list.getBlacklistedFiles ();
         initiallyBlacklistedFiles = std::set<juce::String> (blacklisted.begin (),
                                                             blacklisted.end ());

         // If the filesOrIdentifiersToScan argument isn't empty, we should only scan these
         // If the path is empty, then paths aren't used for this format.
         pathList.setPath (getPluginSearchPaths ());
         pathList.setSize (500, 300);

         pathChooserWindow->addCustomComponent (&pathList);
         pathChooserWindow->addButton (
            TRANS ("Scan"), 1, juce::KeyPress (juce::KeyPress::returnKey));
         pathChooserWindow->addButton (
            TRANS ("Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));

         pathChooserWindow->setVisible (true);
         pathChooserWindow->enterModalState (true,
                                             juce::ModalCallbackFunction::forComponent (
                                                startScanCallback, &*pathChooserWindow, this),
                                             false);
      }

      ~Scanner () override
      {
         if (pool != nullptr) {
            pool->removeAllJobs (true, 60000);
            pool.reset ();
         }
      }

      void thing ();

   private:
      SettingsPage& m_parent;
      std::unique_ptr<juce::PluginDirectoryScanner> scanner;
      Box<juce::AlertWindow> pathChooserWindow, progressWindow;
      juce::FileSearchPathListComponent pathList;
      juce::String pluginBeingScanned;
      double progress = 0;
      bool timerReentrancyCheck = false;
      std::atomic<bool> finished { false };
      std::unique_ptr<juce::ThreadPool> pool;
      std::set<juce::String> initiallyBlacklistedFiles;
      Box<juce::AudioPluginFormat> m_format;

      static void startScanCallback (int result, juce::AlertWindow* alert, Scanner* scanner)
      {
         if (alert != nullptr && scanner != nullptr) {
            if (result != 0) {
               scanner->warnUserAboutStupidPaths ();
            } else {
               scanner->finishedScan ();
            }
         }
      }

      // Try to dissuade people from to scanning their entire C: drive, or other system folders.
      void warnUserAboutStupidPaths ()
      {
         for (int i = 0; i < pathList.getPath ().getNumPaths (); ++i) {
            auto f = pathList.getPath ()[i];

            if (isStupidPath (f)) {
               juce::AlertWindow::showOkCancelBox (
                  juce::MessageBoxIconType::WarningIcon,
                  TRANS ("Plugin Scanning"),
                  TRANS ("If you choose to scan folders that contain non-plugin files, "
                         "then scanning may take a long time, and can cause crashes when "
                         "attempting to load unsuitable files.")
                     + juce::newLine
                     + TRANS ("Are you sure you want to scan the folder \"XYZ\"?")
                          .replace ("XYZ", f.getFullPathName ()),
                  TRANS ("Scan"),
                  juce::String (),
                  nullptr,
                  juce::ModalCallbackFunction::create (warnAboutStupidPathsCallback, this));
               return;
            }
         }

         startScan ();
      }

      static bool isStupidPath (const juce::File& f)
      {
         juce::Array<juce::File> roots;
         juce::File::findFileSystemRoots (roots);

         if (roots.contains (f)) {
            return true;
         }

         juce::File::SpecialLocationType paths_that_would_be_stupid_to_scan[] = {
            juce::File::globalApplicationsDirectory,
            juce::File::userHomeDirectory,
            juce::File::userDocumentsDirectory,
            juce::File::userDesktopDirectory,
            juce::File::tempDirectory,
            juce::File::userMusicDirectory,
            juce::File::userMoviesDirectory,
            juce::File::userPicturesDirectory
         };

         return std::ranges::any_of (paths_that_would_be_stupid_to_scan, [&] (auto& location) {
            auto silly_folder = juce::File::getSpecialLocation (location);
            return f == silly_folder || silly_folder.isAChildOf (f);
         });
      }

      static void warnAboutStupidPathsCallback (int result, Scanner* scanner)
      {
         if (result != 0) {
            scanner->startScan ();
         } else {
            scanner->finishedScan ();
         }
      }

      void startScan ()
      {
         auto& plugin_manager = GlobalStateManager::getPluginManager ();
         pathChooserWindow->setVisible (false);
         scanner = std::make_unique<juce::PluginDirectoryScanner> (
            plugin_manager.getKnownPluginList (),
            *m_format,
            pathList.getPath (),
            true,
            plugin_manager.getRecentlyCrashedPluginsFile (),
            false);

         progressWindow->addButton (TRANS ("Cancel"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
         progressWindow->addProgressBarComponent (progress);
         progressWindow->enterModalState ();

         //    pool.reset (new juce::ThreadPool (numThreads));

         //    for (int i = numThreads; --i >= 0;)
         //       pool->addJob (new ScanJob (*this), true);
         // }

         startTimer (20);
      }

      void finishedScan ()
      {
         auto& plugin_manager = GlobalStateManager::getPluginManager ();

         const auto blacklisted = plugin_manager.getKnownPluginList ().getBlacklistedFiles ();
         std::set<juce::String> allBlacklistedFiles (blacklisted.begin (), blacklisted.end ());

         std::vector<juce::String> newBlacklistedFiles;
         std::set_difference (allBlacklistedFiles.begin (),
                              allBlacklistedFiles.end (),
                              initiallyBlacklistedFiles.begin (),
                              initiallyBlacklistedFiles.end (),
                              std::back_inserter (newBlacklistedFiles));

         for (auto& item : newBlacklistedFiles) {
            plugin_manager.getKnownPluginList ().addToBlacklist (item);
         }

         stopTimer ();
         thing ();
      }

      void timerCallback () override
      {
         if (timerReentrancyCheck)
            return;

         progress = scanner->getProgress ();

         if (pool == nullptr) {
            const juce::ScopedValueSetter<bool> setter (timerReentrancyCheck, true);

            if (doNextScan ())
               startTimer (20);
         }

         if (!progressWindow->isCurrentlyModal ())
            finished = true;

         if (finished) {
            progressWindow.reset ();
            finishedScan ();
         } else {
            progressWindow->setMessage (TRANS ("Testing") + ":\n\n" + pluginBeingScanned);
         }
      }

      bool doNextScan ()
      {
         if (scanner->scanNextFile (true, pluginBeingScanned))
            return true;

         finished = true;
         return false;
      }

      // struct ScanJob: public juce::ThreadPoolJob
      // {
      //    ScanJob (Scanner& s) : ThreadPoolJob ("pluginscan"), scanner (s) {}

      //    JobStatus runJob ()
      //    {
      //       while (scanner.doNextScan () && !shouldExit ()) {}

      //       return jobHasFinished;
      //    }

      //    Scanner& scanner;

      //    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScanJob)
      // };

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scanner)
   };

   class SettingsPage: public juce::Component
   {
   public:
      SettingsPage ()
      {
         setName ("Settings");
         m_scan_plugin_button.setButtonText ("Scan VST3 plugins");
         m_scan_plugin_button.setColour (juce::TextButton::ColourIds::buttonColourId,
                                         juce::Colour::fromRGB (46, 46, 45));
         m_scan_plugin_button.setColour (juce::TextButton::ColourIds::textColourOnId,
                                         juce::Colour::fromRGB (194, 192, 190));

         m_scan_plugin_button.onClick = [this] {
            m_scanner = std::make_unique<Scanner> (*this, "Plugin Scanner", "what goes here");
         };

         addAndMakeVisible (m_scan_plugin_button);
      }

      void paint (juce::Graphics& g) override
      {
         using namespace juce;
         g.fillAll (juce::Colour::fromRGB (194, 192, 190));
      }

      void resized () override
      {
         auto bounds = getBounds ();
         m_scan_plugin_button.setBounds (bounds.removeFromTop (30));
      }

      void deleteScanner () { m_scanner.reset (); }

   private:
      Box<juce::PluginListComponent> m_plc;
      juce::TextButton m_scan_plugin_button;

      Box<Scanner> m_scanner;
   };

   void Scanner::thing ()
   {
      juce::MessageManager::callAsync ([this] { m_parent.deleteScanner (); });
   }

   // ===============================================================================

   //=========================================================================================

   ButtonBar::ButtonBar () : m_shadower (juce::DropShadow (juce::Colours::black, 100, { 0, 0 }))
   {
      m_shadower.setOwner (this);
   }

   void ButtonBar::paint (juce::Graphics& g)
   {
      using namespace juce;
      auto bounds = getLocalBounds ().toFloat ();
      Line<float> l = { bounds.getTopRight (), bounds.getBottomRight () };
      g.setColour (Colours::white.darker (2.0));
      g.drawLine (l, 1.2f);
   }

   void ButtonBar::resized ()
   {
      using namespace juce;
      auto fb = FlexBox (FlexBox::Direction::column,
                         FlexBox::Wrap::noWrap,
                         FlexBox::AlignContent::center,
                         FlexBox::AlignItems::center,
                         FlexBox::JustifyContent::center);
      auto bounds = getLocalBounds ();
      for (auto& button : m_buttons) {
         fb.items.add (
            FlexItem (*button).withWidth (bounds.getWidth ()).withHeight (bounds.getWidth ()));
      }
      fb.performLayout (bounds);
   }

   void ButtonBar::addButton (Box<juce::Button> button)
   {
      addAndMakeVisible (*m_buttons.emplace_back (std::move (button)));
   }

   void ButtonBar::setActiveButton (juce::Button* button)
   {
      if (m_activeButton) {
         m_activeButton->setToggleState (false, juce::NotificationType::dontSendNotification);
      }
      if (button) {
         button->setToggleState (true, juce::NotificationType::dontSendNotification);
      }
      m_activeButton = button;
   }

   usize ButtonBar::getButtonIndex (juce::Button* button)
   {
      using namespace ranges;
      for (auto idx = 0; idx < m_buttons.size (); idx++) {
         if (m_buttons[idx].get () == button) {
            return idx;
         }
      }
   }

   //=========================================================================================

   SideBarContentPanel::SideBarContentPanel () {}
   void SideBarContentPanel::paint (juce::Graphics& g)
   {
      using namespace juce;
      auto bounds = getLocalBounds ();

      auto titleBounds = bounds.removeFromTop (28);
      g.setColour (Colour::fromRGB (79, 79, 79));
      Path p;
      p.addRectangle (titleBounds);
      g.fillPath (p);

      titleBounds.removeFromLeft (10);

      g.setColour (Colours::white);
      if (m_activeComponent) {
         g.drawText (m_activeComponent->getName ().toUpperCase (),
                     titleBounds,
                     Justification::centredLeft,
                     true);
      }
   }
   void SideBarContentPanel::paintOverChildren (juce::Graphics& g)
   {
      using namespace juce;
      auto bounds = getLocalBounds ().toFloat ();
      Line<float> l = { bounds.getTopRight (), bounds.getBottomRight () };
      g.setColour (Colours::white.darker (2.0));
      g.drawLine (l, 1.2f);
   }

   void SideBarContentPanel::resized ()
   {
      auto bounds = getLocalBounds ();
      bounds.removeFromTop (28);

      if (m_activeComponent) {
         m_activeComponent->setBounds (bounds);
      }
   }

   void SideBarContentPanel::buttonClicked (juce::Button* buttonPtr)
   {
      for (auto& [ptr, component] : m_components) {
         if (ptr == buttonPtr) {
            if (m_activeComponent == &*component) {
               m_activeComponent = nullptr;
            } else {
               if (m_activeComponent != nullptr) {
                  removeChildComponent (m_activeComponent);
               }
               m_activeComponent = &*component;
               addAndMakeVisible (m_activeComponent);
            }
         }
      }
      sendChangeMessage ();
      resized ();
      repaint ();
   }

   void SideBarContentPanel::addContentComponent (juce::Button* buttonPtr,
                                                  Box<juce::Component> content)
   {
      auto ptrContentPair = std::make_pair (static_cast<void*> (buttonPtr), std::move (content));
      m_components.push_back (std::move (ptrContentPair));
   }

   juce::Component* SideBarContentPanel::getActiveContent () { return m_activeComponent; }

   //=========================================================================================

   const u32 BUTTOM_BAR_WIDTH = 40;

   TestSideBar::TestSideBar ()
   {
      juce::Path p;
      Box<juce::Component> content;

#if 0
      p.loadPathFromData (data::icons::fileIconPath, sizeof (data::icons::fileIconPath));
      content = Box<juce::Component> (new TestComponent (juce::String ("Presets")));
      addPage ("Presets", std::move (p), std::move (content));

      p.loadPathFromData (data::icons::devicesIconPath, sizeof (data::icons::devicesIconPath));
      content = Box<juce::Component> (new TestComponent (juce::String ("Devices")));
      addPage ("Devices", std::move (p), std::move (content));

#endif
      p.loadPathFromData (data::icons::extensionsIconPath, sizeof (data::icons::extensionsIconPath));
      content = Box<juce::Component> (new PluginTreeView ());
      addPage ("Tree Veiw", std::move (p), std::move (content));

      p.loadPathFromData (data::icons::pluginIconPath, sizeof (data::icons::pluginIconPath));
      content = Box<juce::Component> (new PluginList ());
      addPage ("Plugins", std::move (p), std::move (content));

      p.loadPathFromData (data::icons::settingsIconPath, sizeof (data::icons::settingsIconPath));
      content = Box<juce::Component> (new SettingsPage ());
      addPage ("Settings", std::move (p), std::move (content));
   }

   void TestSideBar::addPage (juce::String name, juce::Path iconPath, Box<juce::Component> content)
   {
      using namespace juce;
      auto button = Box<juce::Button> (new SidebarButton (m_buttonBar, name, std::move (iconPath)));
      button->addListener (&m_contentPanel);
      m_contentPanel.addContentComponent (&*button, std::move (content));
      m_buttonBar.addButton (static_cast<Box<juce::Button>> (std::move (button)));
   }

   SideBarContentPanel& TestSideBar::getContentPanel () { return m_contentPanel; }
   ButtonBar& TestSideBar::getButtonBar () { return m_buttonBar; }
} // namespace cntx::node_graph
