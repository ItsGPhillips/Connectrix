#pragma once

namespace cntx::node_graph
{
   struct OpenSidePanel
   {
      usize index;
   };

   using SidebarMessage = std::variant<OpenSidePanel>;

   class ButtonBar: public juce::Component
   {
   public:
      ButtonBar ();
      void paint (juce::Graphics&) override;
      void resized () override;

      void addButton (Box<juce::Button> button);
      void setActiveButton (juce::Button* button);
      usize getButtonIndex (juce::Button*);

   private:
      Vec<Box<juce::Button>> m_buttons;
      juce::Button* m_activeButton = nullptr;
      juce::DropShadower m_shadower;
   };

   class SideBarContentPanel: public juce::Component,
                              public juce::ChangeBroadcaster,
                              public juce::Button::Listener,
                              public MessageQueue<SidebarMessage>::Listener
   {
   public:
      SideBarContentPanel ();
      void paint (juce::Graphics&) override;
      void paintOverChildren (juce::Graphics&) override;
      void resized () override;
      void buttonClicked (juce::Button* button) override;
      juce::Component* getActiveContent ();
      void addContentComponent (juce::Button* buttonPtr, Box<juce::Component> content);

      void messageQueueListenerCallback (const SidebarMessage& message) override
      {
         std::visit ([] (const OpenSidePanel& cmd) { LOG_DEBUG (cmd.index); }, message);
      }

   private:
      // void* is the ptr to the button
      std::vector<std::pair<void*, Box<juce::Component>>> m_components;
      juce::Component* m_activeComponent = nullptr;
   };

   class TestSideBar
   {
   public:
      TestSideBar ();
      void addPage (juce::String name, juce::Path iconPath, Box<juce::Component> content);
      SideBarContentPanel& getContentPanel ();
      ButtonBar& getButtonBar ();

   private:
      ButtonBar m_buttonBar;
      SideBarContentPanel m_contentPanel;
   };
} // namespace cntx::node_graph_editor
