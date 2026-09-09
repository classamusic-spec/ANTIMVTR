#include "AntiMatrTheme.h"

#if defined (ANTIMATR_HAS_FONTS) && ANTIMATR_HAS_FONTS
 #include "AntiMatrAssets.h"
#endif

namespace am::ui
{

namespace
{
    Theme::Typefaces loadTypefaces()
    {
        Theme::Typefaces t;
       #if defined (ANTIMATR_HAS_FONTS) && ANTIMATR_HAS_FONTS
        auto load = [] (const char* data, int size) -> juce::Typeface::Ptr
        {
            if (data == nullptr || size < 1024) return nullptr;
            return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        };
        t.display       = load (AntiMatrAssets::MichromaRegular_ttf,     AntiMatrAssets::MichromaRegular_ttfSize);
        t.label         = load (AntiMatrAssets::BeVietnamProRegular_ttf,  AntiMatrAssets::BeVietnamProRegular_ttfSize);
        t.labelMedium   = load (AntiMatrAssets::BeVietnamProMedium_ttf,   AntiMatrAssets::BeVietnamProMedium_ttfSize);
        t.labelSemiBold = load (AntiMatrAssets::BeVietnamProSemiBold_ttf, AntiMatrAssets::BeVietnamProSemiBold_ttfSize);
        t.mono          = load (AntiMatrAssets::SpaceMonoRegular_ttf,     AntiMatrAssets::SpaceMonoRegular_ttfSize);
        t.embedded = t.display != nullptr && t.label != nullptr && t.mono != nullptr;
        if (t.labelMedium == nullptr)   t.labelMedium   = t.label;
        if (t.labelSemiBold == nullptr) t.labelSemiBold = t.labelMedium;
       #endif
        return t;
    }
}

const Theme::Typefaces& Theme::typefaces()
{
    static const Typefaces instance = loadTypefaces();
    return instance;
}

juce::Font Theme::make (const juce::Typeface::Ptr& typeface, float height, float tracking, bool boldFallback, bool monoFallback)
{
    height = juce::jmax (4.0f, height);
    juce::Font f = typeface != nullptr
        ? juce::Font (juce::FontOptions().withTypeface (typeface).withHeight (height))
        : juce::Font (juce::FontOptions (monoFallback ? juce::Font::getDefaultMonospacedFontName() : juce::Font::getDefaultSansSerifFontName(),
                                         height, boldFallback ? juce::Font::bold : juce::Font::plain));
    if (tracking != 0.0f) f = f.withExtraKerningFactor (tracking);
    return f;
}

} // namespace am::ui
