#pragma once

#include <core.h>
using namespace core;
#include <JuceHeader.h>

#include "message_queue.h"

#define CREATE_IDENT(name) static const juce::Identifier name = juce::Identifier (#name);

template <class T>
concept TriviallyConvertable = requires
{
   std::is_trivially_copyable<T> ();
   std::is_trivially_constructible<T> ();
   std::is_trivially_destructible<T> ();
};

template <TriviallyConvertable T, TriviallyConvertable U>
struct juce::VariantConverter<std::pair<T, U>>
{
   static std::pair<T, U> fromVar (const juce::var& v)
   {
      std::pair<T, U> p;
      if (v.isBinaryData ()) {
         auto* mem_block = v.getBinaryData ();
         void* data = mem_block->getData ();
         jassert (sizeof (std::pair<T, U>) == mem_block->getSize ());
         std::memcpy (&p, data, sizeof (std::pair<T, U>));
      } else {
         // var must be binary data
         jassertfalse;
      }
      return p;
   }
   static juce::var toVar (const std::pair<T, U>& p)
   {
      return juce::var (static_cast<const void*> (&p), sizeof (p));
   }
};

template <class ValueType>
struct juce::VariantConverter<juce::Point<ValueType>>
{
   static juce::Point<ValueType> fromVar (const juce::var& v)
   {
      auto [x, y] = juce::VariantConverter<std::pair<ValueType, ValueType>>::fromVar (v);
      return juce::Point<ValueType> (x, y);
   }
   static juce::var toVar (const juce::Point<ValueType>& p)
   {
      return juce::VariantConverter<std::pair<ValueType, ValueType>>::toVar ({ p.x, p.y });
   }
};

namespace cntx
{
   using CachedPointValue = juce::CachedValue<juce::Point<f32>>;
}