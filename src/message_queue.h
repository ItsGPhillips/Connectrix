#pragma once
#include <typeinfo>

namespace cntx
{
   template <class T>
   class MessageQueue
   {
   public:
      class Listener
      {
      public:
         Listener ();
         virtual ~Listener ();
         virtual void messageQueueListenerCallback (const T&) = 0;

      private:
      };

      MessageQueue () = default;
      ~MessageQueue ();

      void addListener (Listener* listener);
      void removeListener (Listener* listener);

      void postMessage (Rc<T>);
      void postMessage (T&&);

      static MessageQueue<T>* getInstance ();
      static MessageQueue<T>* getInstanceWithoutCreating ();
      static void deleteInstance ();

   private:
      inline static MessageQueue<T>* INSTANCE = nullptr;
      juce::ListenerList<Listener> m_listeners;
   };

   template <class T>
   inline MessageQueue<T>::~MessageQueue ()
   {}

   template <class T>
   void MessageQueue<T>::addListener (MessageQueue<T>::Listener* listener)
   {
      m_listeners.add (listener);
   }

   template <class T>
   MessageQueue<T>::Listener::Listener ()
   {
      if (auto* instance = MessageQueue<T>::getInstance ()) {
         instance->addListener (this);
      }
   }

   template <class T>
   MessageQueue<T>::Listener::~Listener ()
   {
      if (auto* instance = MessageQueue<T>::getInstanceWithoutCreating ()) {
         instance->removeListener (this);
      }
   }

   template <class T>
   inline void MessageQueue<T>::removeListener (Listener* listener)
   {
      m_listeners.remove (listener);
   }

   template <class T>
   inline void MessageQueue<T>::postMessage (Rc<T> message)
   {
      juce::MessageManager::callAsync ([=, this] {
         m_listeners.call (
            [=] (Listener& callback) { callback.messageQueueListenerCallback (*message); });
      });
   }

   template <class T>
   inline void MessageQueue<T>::postMessage (T&& message)
   {
      postMessage (std::make_shared<T> (std::forward<T> (message)));
   }

   class Test: public MessageQueue<f32>::Listener,
               public MessageQueue<char>::Listener
   {
   public:
      void messageQueueListenerCallback (const f32&) override {}
      void messageQueueListenerCallback (const char&) override {}

   private:
   };

   template <class T>
   inline MessageQueue<T>* MessageQueue<T>::getInstance ()
   {
      if (INSTANCE == nullptr) {
         INSTANCE = new MessageQueue<T> ();
      }
      return INSTANCE;
   }

   template <class T>
   inline MessageQueue<T>* MessageQueue<T>::getInstanceWithoutCreating ()
   {
      return INSTANCE;
   }

   template <class T>
   inline void MessageQueue<T>::deleteInstance ()
   {
      if (INSTANCE != nullptr) {
         delete INSTANCE;
         INSTANCE = nullptr;
      }
      return;
   }

} // namespace cntx
