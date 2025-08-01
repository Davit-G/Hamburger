#pragma once

#include "LookAndFeel/HamburgerLAF.h"

class AviaryLogo : public juce::Component
{
public:
    AviaryLogo()
    {
        auto aviaryString = R"svgDELIM(
        <svg viewBox="0 0 186 48" xmlns="http://www.w3.org/2000/svg">
        <path d="m22.04 16.16h1.56l11.44 27.84h-1.6l-3.76-9.2h-13.72l-3.8 9.2h-1.56l11.44-27.84zm-5.48 17.2h12.52l-6.28-15.28-6.24 15.28zm25.169-17.2 10.64 25.88 10.64-25.88h1.56l-11.44 27.84h-1.56l-11.44-27.84h1.6zm49.329 0v1.48h-8.32v24.88h8.32v1.48h-18.04v-1.48h8.28v-24.88h-8.28v-1.48h18.04zm19.809 0h1.56l11.44 27.84h-1.6l-3.76-9.2h-13.72l-3.8 9.2h-1.5599l11.44-27.84zm-5.48 17.2h12.52l-6.28-15.28-6.24 15.28zm41.89-16.28c1.6 0.7467 2.853 1.8533 3.76 3.32 0.933 1.4667 1.4 3.0933 1.4 4.88 0 2.08-0.64 3.9467-1.92 5.6-1.254 1.6267-2.867 2.7067-4.84 3.24l7.08 9.88h-1.76l-6.84-9.6-1.28 0.04h-10.04v9.56h-1.48v-27.84h10.6c1.173 0 2.173 0.08 3 0.24 0.853 0.1333 1.626 0.36 2.32 0.68zm-4 15.88c1.386 0 2.666-0.3467 3.84-1.04 1.173-0.6933 2.106-1.6267 2.8-2.8 0.693-1.1733 1.04-2.4533 1.04-3.84 0-1.5467-0.414-2.9467-1.24-4.2-0.8-1.2533-1.92-2.1867-3.36-2.8-1.04-0.4267-2.507-0.64-4.4-0.64h-9.12v15.32h10.44zm17.729-16.8 9.88 14.88 9.8-14.88h1.76l-10.84 16.44v11.4h-1.48v-11.4l-10.88-16.44h1.76z" fill="#575757"/>
        <path d="m18.04 12.16h1.56l11.44 27.84h-1.6l-3.76-9.2h-13.72l-3.8 9.2h-1.56l11.44-27.84zm-5.48 17.2h12.52l-6.28-15.28-6.24 15.28zm25.169-17.2 10.64 25.88 10.64-25.88h1.56l-11.44 27.84h-1.56l-11.44-27.84h1.6zm49.329 0v1.48h-8.32v24.88h8.32v1.48h-18.04v-1.48h8.28v-24.88h-8.28v-1.48h18.04zm19.809 0h1.56l11.44 27.84h-1.6l-3.76-9.2h-13.72l-3.7999 9.2h-1.56l11.44-27.84zm-5.48 17.2h12.52l-6.28-15.28-6.24 15.28zm41.89-16.28c1.6 0.7467 2.853 1.8533 3.76 3.32 0.933 1.4667 1.4 3.0933 1.4 4.88 0 2.08-0.64 3.9467-1.92 5.6-1.254 1.6267-2.867 2.7067-4.84 3.24l7.08 9.88h-1.76l-6.84-9.6-1.28 0.04h-10.04v9.56h-1.48v-27.84h10.6c1.173 0 2.173 0.08 3 0.24 0.853 0.1333 1.626 0.36 2.32 0.68zm-4 15.88c1.386 0 2.666-0.3467 3.84-1.04 1.173-0.6933 2.106-1.6267 2.8-2.8 0.693-1.1733 1.04-2.4533 1.04-3.84 0-1.5467-0.414-2.9467-1.24-4.2-0.8-1.2533-1.92-2.1867-3.36-2.8-1.04-0.4267-2.507-0.64-4.4-0.64h-9.12v15.32h10.44zm17.729-16.8 9.88 14.88 9.8-14.88h1.76l-10.84 16.44v11.4h-1.48v-11.4l-10.88-16.44h1.76z" fill="#A7A7A7"/>
        <path d="m14.04 8.16h1.56l11.44 27.84h-1.6l-3.76-9.2h-13.72l-3.8 9.2h-1.56l11.44-27.84zm-5.48 17.2h12.52l-6.28-15.28-6.24 15.28zm25.169-17.2 10.64 25.88 10.64-25.88h1.56l-11.44 27.84h-1.56l-11.44-27.84h1.6zm49.329 0v1.48h-8.32v24.88h8.32v1.48h-18.04v-1.48h8.28v-24.88h-8.28v-1.48h18.04zm19.809 0h1.56l11.44 27.84h-1.6l-3.76-9.2h-13.72l-3.8 9.2h-1.56l11.44-27.84zm-5.4799 17.2h12.52l-6.28-15.28-6.2399 15.28zm41.89-16.28c1.6 0.74667 2.853 1.8533 3.76 3.32 0.933 1.4667 1.4 3.0933 1.4 4.88 0 2.08-0.64 3.9467-1.92 5.6-1.254 1.6267-2.867 2.7067-4.84 3.24l7.08 9.88h-1.76l-6.84-9.6-1.28 0.04h-10.04v9.56h-1.48v-27.84h10.6c1.173 0 2.173 0.08 3 0.24 0.853 0.13333 1.626 0.36 2.32 0.68zm-4 15.88c1.386 0 2.666-0.3467 3.84-1.04 1.173-0.6933 2.106-1.6267 2.8-2.8 0.693-1.1733 1.04-2.4533 1.04-3.84 0-1.5467-0.414-2.9467-1.24-4.2-0.8-1.2533-1.92-2.1867-3.36-2.8-1.04-0.42667-2.507-0.64-4.4-0.64h-9.12v15.32h10.44zm17.729-16.8 9.88 14.88 9.8-14.88h1.76l-10.84 16.44v11.4h-1.48v-11.4l-10.88-16.44h1.76z" fill="#fff"/>
        </svg>
        )svgDELIM";
        
        auto parsedLogoStringAviary{XmlDocument::parse(String(aviaryString))};
        jassert(parsedLogoStringAviary != nullptr);
        drawableAviary = Drawable::createFromSVG(*parsedLogoStringAviary);
        jassert(drawableAviary != nullptr);

        setSize(drawableAviary->getDrawableBounds().getWidth(), drawableAviary->getDrawableBounds().getHeight());

        setMouseCursor(MouseCursor::PointingHandCursor);

        addAndMakeVisible(*drawableAviary);
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        DBG("Mouse up");
        URL("https://aviaryaudio.com").launchInDefaultBrowser();
    }

    void resized() override
    {
        drawableAviary->setBounds(getLocalBounds());
    }

private:
    std::unique_ptr<juce::Drawable> drawableAviary = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AviaryLogo)
};



class Info : public juce::Component
{
public:
    Info(AudioPluginAudioProcessor &p) : changePresetFolderButton("Change Preset Folder")
    {

        auto logoString = R"svgDELIM(
        <svg viewBox="0 0 297 45" xmlns="http://www.w3.org/2000/svg">
        <path d="m1 32.75c0-0.9665 0.7835-1.75 1.75-1.75h28.5c0.9665 0 1.75 0.7835 1.75 1.75 0 2.8995-2.3505 5.25-5.25 5.25h-21.5c-2.8995 0-5.25-2.3505-5.25-5.25z" fill="#FFD056" stroke="#000"/>
        <rect x="3" y="26" width="28" height="4" rx="2" fill="#AE7348" stroke="#8B4C1E"/>
        <rect x="3" y="23" width="28" height="4" rx="2" fill="#A5FF8E" stroke="#81FF62"/>
        <rect x="3" y="20" width="28" height="4" rx="2" fill="#FF7B7B" stroke="#FF4A4A"/>
        <path d="m16.86 8c-10.372 0-14.669 4.0049-15.774 8.938-0.24314 1.0862 1e-5 2.062 1.1043 2.062h29.407c1.2745 0 1.6252-0.7929 1.2745-2.062-1.2745-4.6128-6.0066-8.938-16.012-8.938z" fill="#FFD056" stroke="#000"/>
        <path d="m63.708 37.824c-0.3227 0.088-0.836 0.176-1.54 0.264-0.6747 0.088-1.3493 0.132-2.024 0.132s-1.276-0.0587-1.804-0.176c-0.4987-0.088-0.924-0.264-1.276-0.528s-0.616-0.6307-0.792-1.1-0.264-1.0853-0.264-1.848v-6.864h-8.448v10.12c-0.3227 0.088-0.836 0.176-1.54 0.264-0.6747 0.088-1.3493 0.132-2.024 0.132s-1.276-0.0587-1.804-0.176c-0.4987-0.088-0.924-0.264-1.276-0.528s-0.616-0.6307-0.792-1.1-0.264-1.0853-0.264-1.848v-23.232c0.3227-0.0587 0.836-0.132 1.54-0.22 0.704-0.1173 1.3787-0.176 2.024-0.176 0.6747 0 1.2613 0.0587 1.76 0.176 0.528 0.088 0.968 0.264 1.32 0.528s0.616 0.6307 0.792 1.1 0.264 1.0853 0.264 1.848v7.04h8.448v-10.296c0.3227-0.0587 0.836-0.132 1.54-0.22 0.704-0.1173 1.3787-0.176 2.024-0.176 0.6747 0 1.2613 0.0587 1.76 0.176 0.528 0.088 0.968 0.264 1.32 0.528s0.616 0.6307 0.792 1.1 0.264 1.0853 0.264 1.848v23.232zm11.39-25.696c0.4987-0.2933 1.2467-0.572 2.244-0.836 1.0267-0.264 2.0533-0.396 3.08-0.396 1.4667 0 2.7573 0.22 3.872 0.66 1.144 0.4107 1.8773 1.0707 2.2 1.98 0.528 1.5253 1.1 3.3 1.716 5.324s1.232 4.1213 1.848 6.292c0.616 2.1413 1.2173 4.268 1.804 6.38 0.5867 2.0827 1.1 3.9307 1.54 5.544-0.352 0.4107-0.88 0.7333-1.584 0.968-0.704 0.264-1.54 0.396-2.508 0.396-0.704 0-1.3053-0.0587-1.804-0.176-0.4693-0.1173-0.8653-0.2933-1.188-0.528-0.2933-0.2347-0.5427-0.5427-0.748-0.924-0.176-0.3813-0.3373-0.8213-0.484-1.32l-0.792-3.036h-7.964c-0.2347 0.88-0.4693 1.7893-0.704 2.728s-0.4693 1.848-0.704 2.728c-0.4107 0.1467-0.8507 0.264-1.32 0.352-0.44 0.1173-0.9827 0.176-1.628 0.176-1.496 0-2.6107-0.2493-3.344-0.748-0.704-0.528-1.056-1.2467-1.056-2.156 0-0.4107 0.0587-0.8213 0.176-1.232s0.2493-0.8947 0.396-1.452c0.2053-0.792 0.484-1.7747 0.836-2.948 0.3813-1.1733 0.792-2.4347 1.232-3.784 0.44-1.3787 0.9093-2.7867 1.408-4.224s0.968-2.7867 1.408-4.048c0.44-1.2907 0.836-2.4347 1.188-3.432 0.3813-0.9973 0.6747-1.76 0.88-2.288zm5.28 5.676c-0.4107 1.232-0.88 2.6107-1.408 4.136-0.4987 1.496-0.9533 3.0213-1.364 4.576h5.456c-0.4107-1.584-0.836-3.124-1.276-4.62-0.4107-1.5253-0.792-2.8893-1.144-4.092h-0.264zm35.066 13.64c-0.381 0.3227-0.88 0.5867-1.496 0.792-0.586 0.176-1.276 0.264-2.068 0.264-1.056 0-1.936-0.132-2.64-0.396-0.674-0.264-1.114-0.7187-1.32-1.364-0.821-2.5813-1.496-4.664-2.024-6.248-0.528-1.6133-0.938-3.0213-1.232-4.224h-0.22c-0.088 1.7307-0.161 3.3147-0.22 4.752-0.029 1.408-0.058 2.7867-0.088 4.136 0 1.3493-0.029 2.7133-0.088 4.092-0.029 1.3787-0.088 2.904-0.176 4.576-0.381 0.1467-0.88 0.264-1.496 0.352-0.616 0.1173-1.232 0.176-1.848 0.176-1.2902 0-2.3022-0.2053-3.0356-0.616-0.704-0.4107-1.056-1.1587-1.056-2.244l1.276-22.44c0.2347-0.3813 0.748-0.792 1.54-1.232 0.7916-0.44 1.9066-0.66 3.3436-0.66 1.555 0 2.772 0.2493 3.652 0.748 0.88 0.4693 1.526 1.2467 1.936 2.332 0.264 0.704 0.543 1.5107 0.836 2.42 0.323 0.9093 0.631 1.848 0.924 2.816 0.323 0.968 0.631 1.936 0.924 2.904 0.294 0.9387 0.558 1.7893 0.792 2.552h0.22c0.704-2.464 1.408-4.84 2.112-7.128 0.704-2.3173 1.35-4.2533 1.936-5.808 0.44-0.2347 0.998-0.4253 1.672-0.572 0.704-0.176 1.482-0.264 2.332-0.264 1.438 0 2.611 0.22 3.52 0.66 0.91 0.4107 1.438 1.012 1.584 1.804 0.118 0.5867 0.235 1.496 0.352 2.728 0.147 1.2027 0.294 2.5667 0.44 4.092 0.147 1.5253 0.294 3.1387 0.44 4.84 0.147 1.7013 0.279 3.344 0.396 4.928 0.147 1.5547 0.264 2.992 0.352 4.312 0.088 1.2907 0.147 2.288 0.176 2.992-0.498 0.2933-1.026 0.4987-1.584 0.616-0.528 0.1467-1.217 0.22-2.068 0.22-1.114 0-2.053-0.1907-2.816-0.572-0.762-0.3813-1.173-1.1293-1.232-2.244-0.176-3.1973-0.293-6.072-0.352-8.624-0.029-2.552-0.088-4.6933-0.176-6.424h-0.22c-0.293 1.144-0.718 2.5813-1.276 4.312-0.557 1.7307-1.232 3.9453-2.024 6.644z" fill="#FFB55F"/>
        <path d="m139.81 32.5c0.41 0.1173 0.938 0.2053 1.584 0.264 0.645 0.0293 1.261 0.044 1.848 0.044 1.173 0 2.141-0.2347 2.904-0.704 0.792-0.4987 1.188-1.2613 1.188-2.288 0-0.968-0.323-1.6573-0.968-2.068-0.646-0.4107-1.628-0.616-2.948-0.616h-3.608v5.368zm0-10.736h3.212c1.261 0 2.17-0.2347 2.728-0.704 0.586-0.4693 0.88-1.1587 0.88-2.068 0-0.792-0.338-1.4227-1.012-1.892-0.646-0.4693-1.628-0.704-2.948-0.704-0.44 0-0.954 0.0147-1.54 0.044-0.558 0.0293-0.998 0.0733-1.32 0.132v5.192zm2.86 16.94c-0.499 0-1.086-0.0147-1.76-0.044-0.675-0.0293-1.379-0.088-2.112-0.176-0.704-0.088-1.408-0.2053-2.112-0.352-0.704-0.1173-1.35-0.2933-1.936-0.528-1.614-0.616-2.42-1.6867-2.42-3.212v-20.548c0-0.616 0.161-1.0853 0.484-1.408 0.352-0.352 0.821-0.6307 1.408-0.836 0.997-0.352 2.244-0.6013 3.74-0.748 1.496-0.176 3.036-0.264 4.62-0.264 3.754 0 6.644 0.6307 8.668 1.892s3.036 3.212 3.036 5.852c0 1.32-0.382 2.464-1.144 3.432-0.763 0.9387-1.79 1.628-3.08 2.068 1.466 0.4107 2.684 1.1587 3.652 2.244 0.997 1.0853 1.496 2.4493 1.496 4.092 0 2.904-1.086 5.06-3.256 6.468-2.142 1.3787-5.236 2.068-9.284 2.068zm40.53-10.56c0 1.584-0.279 3.036-0.836 4.356-0.528 1.2907-1.305 2.4053-2.332 3.344-1.027 0.9093-2.273 1.6133-3.74 2.112s-3.124 0.748-4.972 0.748-3.505-0.2493-4.972-0.748-2.713-1.2027-3.74-2.112c-1.027-0.9387-1.819-2.0533-2.376-3.344-0.528-1.32-0.792-2.772-0.792-4.356v-16.808c0.323-0.0587 0.836-0.132 1.54-0.22 0.704-0.1173 1.379-0.176 2.024-0.176 0.675 0 1.261 0.0587 1.76 0.176 0.528 0.088 0.968 0.264 1.32 0.528s0.616 0.6307 0.792 1.1 0.264 1.0853 0.264 1.848v13.42c0 1.408 0.381 2.508 1.144 3.3 0.792 0.792 1.804 1.188 3.036 1.188 1.261 0 2.273-0.396 3.036-1.188s1.144-1.892 1.144-3.3v-16.676c0.323-0.0587 0.836-0.132 1.54-0.22 0.704-0.1173 1.379-0.176 2.024-0.176 0.675 0 1.261 0.0587 1.76 0.176 0.528 0.088 0.968 0.264 1.32 0.528s0.616 0.6307 0.792 1.1 0.264 1.0853 0.264 1.848v13.552zm13.247 1.232v8.448c-0.323 0.088-0.836 0.176-1.54 0.264-0.675 0.088-1.35 0.132-2.024 0.132-0.675 0-1.276-0.0587-1.804-0.176-0.499-0.088-0.924-0.264-1.276-0.528-0.323-0.264-0.572-0.6307-0.748-1.1s-0.264-1.0853-0.264-1.848v-20.724c0-0.616 0.161-1.0853 0.484-1.408 0.352-0.352 0.821-0.6307 1.408-0.836 0.997-0.352 2.156-0.6013 3.476-0.748 1.32-0.176 2.728-0.264 4.224-0.264 4.048 0 7.098 0.8507 9.152 2.552 2.053 1.7013 3.08 4.004 3.08 6.908 0 1.8187-0.47 3.3733-1.408 4.664-0.939 1.2907-2.054 2.288-3.344 2.992 1.056 1.4373 2.097 2.7867 3.124 4.048 1.026 1.2613 1.833 2.4053 2.42 3.432-0.294 1.0267-0.836 1.8187-1.628 2.376-0.763 0.528-1.628 0.792-2.596 0.792-0.646 0-1.203-0.0733-1.672-0.22-0.47-0.1467-0.88-0.352-1.232-0.616s-0.675-0.5867-0.968-0.968c-0.294-0.3813-0.572-0.792-0.836-1.232l-3.696-5.94h-2.332zm2.772-5.852c1.114 0 1.994-0.2787 2.64-0.836 0.645-0.5867 0.968-1.4373 0.968-2.552s-0.367-1.9507-1.1-2.508c-0.704-0.5867-1.834-0.88-3.388-0.88-0.44 0-0.792 0.0147-1.056 0.044s-0.558 0.0733-0.88 0.132v6.6h2.816zm40.196 10.12c0 0.9093-0.161 1.584-0.484 2.024-0.322 0.44-0.836 0.8653-1.54 1.276-0.41 0.2347-0.924 0.4693-1.54 0.704-0.616 0.2053-1.29 0.3813-2.024 0.528-0.733 0.176-1.51 0.308-2.332 0.396-0.792 0.088-1.584 0.132-2.376 0.132-2.112 0-4.092-0.2787-5.94-0.836-1.848-0.5867-3.461-1.452-4.84-2.596-1.349-1.1733-2.42-2.6253-3.212-4.356s-1.188-3.74-1.188-6.028c0-2.4053 0.426-4.5027 1.276-6.292 0.88-1.7893 2.024-3.2707 3.432-4.444 1.438-1.2027 3.066-2.0973 4.884-2.684 1.848-0.5867 3.74-0.88 5.676-0.88 2.787 0 4.987 0.4547 6.6 1.364 1.643 0.88 2.464 2.0387 2.464 3.476 0 0.7627-0.19 1.4227-0.572 1.98-0.381 0.5573-0.821 0.9973-1.32 1.32-0.733-0.44-1.657-0.8653-2.772-1.276-1.085-0.44-2.317-0.66-3.696-0.66-2.464 0-4.429 0.704-5.896 2.112-1.437 1.408-2.156 3.3587-2.156 5.852 0 1.3493 0.191 2.5227 0.572 3.52 0.411 0.968 0.954 1.7747 1.628 2.42 0.675 0.616 1.452 1.0853 2.332 1.408 0.88 0.2933 1.819 0.44 2.816 0.44 0.646 0 1.218-0.0587 1.716-0.176 0.499-0.1173 0.88-0.2493 1.144-0.396v-4.136h-4.708c-0.176-0.3227-0.352-0.748-0.528-1.276-0.146-0.528-0.22-1.0853-0.22-1.672 0-1.056 0.235-1.8187 0.704-2.288 0.499-0.4693 1.13-0.704 1.892-0.704h7.436c0.88 0 1.555 0.2493 2.024 0.748 0.499 0.4693 0.748 1.144 0.748 2.024v8.976zm5.063-18.216c0-1.2613 0.367-2.2587 1.1-2.992 0.734-0.7333 1.731-1.1 2.992-1.1h14.52c0.206 0.3227 0.382 0.748 0.528 1.276 0.176 0.528 0.264 1.0853 0.264 1.672 0 1.1147-0.249 1.9067-0.748 2.376-0.469 0.4693-1.1 0.704-1.892 0.704h-9.284v4.092h9.9c0.206 0.3227 0.382 0.748 0.528 1.276 0.176 0.4987 0.264 1.0413 0.264 1.628 0 1.1147-0.234 1.9067-0.704 2.376-0.469 0.4693-1.1 0.704-1.892 0.704h-8.096v4.62h11.308c0.206 0.3227 0.382 0.748 0.528 1.276 0.176 0.528 0.264 1.0853 0.264 1.672 0 1.1147-0.249 1.9213-0.748 2.42-0.469 0.4693-1.1 0.704-1.892 0.704h-12.848c-1.261 0-2.258-0.3667-2.992-1.1-0.733-0.7333-1.1-1.7307-1.1-2.992v-18.612zm31.762 13.948v8.448c-0.323 0.088-0.836 0.176-1.54 0.264-0.675 0.088-1.35 0.132-2.024 0.132-0.675 0-1.276-0.0587-1.804-0.176-0.499-0.088-0.924-0.264-1.276-0.528-0.323-0.264-0.572-0.6307-0.748-1.1s-0.264-1.0853-0.264-1.848v-20.724c0-0.616 0.161-1.0853 0.484-1.408 0.352-0.352 0.821-0.6307 1.408-0.836 0.997-0.352 2.156-0.6013 3.476-0.748 1.32-0.176 2.728-0.264 4.224-0.264 4.048 0 7.098 0.8507 9.152 2.552 2.053 1.7013 3.08 4.004 3.08 6.908 0 1.8187-0.47 3.3733-1.408 4.664-0.939 1.2907-2.054 2.288-3.344 2.992 1.056 1.4373 2.097 2.7867 3.124 4.048 1.026 1.2613 1.833 2.4053 2.42 3.432-0.294 1.0267-0.836 1.8187-1.628 2.376-0.763 0.528-1.628 0.792-2.596 0.792-0.646 0-1.203-0.0733-1.672-0.22-0.47-0.1467-0.88-0.352-1.232-0.616s-0.675-0.5867-0.968-0.968c-0.294-0.3813-0.572-0.792-0.836-1.232l-3.696-5.94h-2.332zm2.772-5.852c1.114 0 1.994-0.2787 2.64-0.836 0.645-0.5867 0.968-1.4373 0.968-2.552s-0.367-1.9507-1.1-2.508c-0.704-0.5867-1.834-0.88-3.388-0.88-0.44 0-0.792 0.0147-1.056 0.044s-0.558 0.0733-0.88 0.132v6.6h2.816z" fill="#FFF730"/>
        </svg>
        )svgDELIM";

        changePresetFolderButton.setLookAndFeel(&hamburgerLAF);

        changePresetFolderButton.setColour(TextButton::buttonColourId, juce::Colours::black);
        changePresetFolderButton.setColour(TextButton::textColourOffId, juce::Colours::white);
        changePresetFolderButton.setColour(TextButton::textColourOnId, juce::Colours::white);

        addAndMakeVisible(changePresetFolderButton);

        auto& presetManager = p.getPresetManager();

        changePresetFolderButton.onClick = [&]() {
            // browse for files
            presetFolderChooser = std::make_unique<juce::FileChooser>("Select a folder to save presets to", Preset::defaultDirectory, "*.*", true);

                auto folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;
 
                presetFolderChooser->launchAsync (folderChooserFlags, [&] (const FileChooser& chooser)
                {
                    auto result = chooser.getResult();

                    if (!result.exists())
                    {
                        DBG("No folder selected");
                        return;
                    }

                    File directory (chooser.getResult());
            
                    auto userSettings = presetManager.appProperties.getUserSettings();

                    userSettings->setValue("presetFolder", directory.getFullPathName());

                    DBG("Setting preset folder to " + directory.getFullPathName());

                    presetManager.setPresetDirectory(directory);
                });
        };

        auto parsedLogoString{XmlDocument::parse(String(logoString))};
        jassert(parsedLogoString != nullptr);
        drawableLogoString = Drawable::createFromSVG(*parsedLogoString);
        jassert(drawableLogoString != nullptr);

#if JUCE_DEBUG
        versionLabel.setText("Version: Debug v" + String(JucePlugin_VersionString) + " (commit " + String(GIT_HASH) + ")", NotificationType::dontSendNotification);
#else
        versionLabel.setText("Version: Release v" + String(JucePlugin_VersionString) + " (" + String(GIT_HASH) + ")", NotificationType::dontSendNotification);
#endif

        auto font = hamburgerLAF.getLabelFont(descriptionLabel);

        versionLabel.setJustificationType(Justification::bottomRight);
        versionLabel.setFont(font);

        descriptionLabel.setJustificationType(Justification::topLeft);
        descriptionLabel.setText("Hamburger is a powerful distortion effect plugin with dynamics controls and equalisation designed for more control. Hamburger takes inspiration from other distortion and analog emulation plugins, and provides a tight set of controls that allow for fast pleasing results. \n \nHuge thanks to Sam (womp), Danburger (Antfactory), Bean, DEPARTURE, Sheaf, HurleybirdJr, Ewan Bristow, Alyssa, Patrhetoric, the dubstep den chaps, all the people in the Aviary Audio discord server and anyone else I've left out, for all the help with development, testing, debugging and contributions.\n\nIf you want to know more about Aviary or join the community, click the aviary logo on the top right (opens a page to https://aviaryaudio.com). \n\nClick anywhere on the screen to return to the plugin", NotificationType::dontSendNotification);
        descriptionLabel.setFont(font);

        drawableLogoString->setInterceptsMouseClicks(false, false);
        descriptionLabel.setInterceptsMouseClicks(false, false);
        versionLabel.setInterceptsMouseClicks(false, false);

        addAndMakeVisible(*drawableLogoString);
        addAndMakeVisible(aviaryLogo);

        addAndMakeVisible(versionLabel);
        addAndMakeVisible(descriptionLabel);
    }

    ~Info() override
    {
        changePresetFolderButton.setLookAndFeel(nullptr);
        setLookAndFeel(nullptr);
        presetFolderChooser.reset();
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        DBG("Mouse clicked");
        getParentComponent()->postCommandMessage(1);
    }

    void paint(Graphics &g) override
    {
        Path p;
        p.addRoundedRectangle(getLocalBounds().reduced(4).toFloat(), 15.0f);
        g.setColour(juce::Colour::fromRGB(0, 0, 0));
        g.fillPath(p);

        auto bounds = getLocalBounds().reduced(20);

        auto logoArea = bounds.removeFromBottom(80);

        g.drawImage(vstLogo, logoArea.removeFromLeft(100).toFloat(), Justification::centred);
        logoArea.removeFromLeft(10);
        g.drawImage(clapLogo, logoArea.removeFromLeft(70).toFloat(), Justification::centred);
        logoArea.removeFromLeft(10);
        g.drawImage(gplLogo, logoArea.removeFromLeft(110).toFloat(), Justification::centred);
        logoArea.removeFromLeft(10);
        g.drawImage(womp, logoArea.removeFromLeft(130).toFloat(), Justification::centred);

        const juce::Image features[4] = {feature1, feature2, feature3, feature4};

        auto featureArea = getLocalBounds().removeFromRight(bounds.getWidth() / 2).reduced(40);

        // pick random feature to display
        auto feature = Random::getSystemRandom().nextInt(Range<int>(0, 4));

        g.drawImage(features[feature], featureArea.toFloat(), Justification::centred);
    }

    void resized() override
    {
        drawableLogoString->setBoundsToFit(getLocalBounds().reduced(20), Justification::topLeft, true);


        versionLabel.setBounds(getLocalBounds().reduced(20).removeFromBottom(20));

        auto infoRegion = getLocalBounds().reduced(19).removeFromLeft(getLocalBounds().getWidth() / 2);
        descriptionLabel.setBounds(infoRegion.withTrimmedTop(80).withTrimmedBottom(80));

        changePresetFolderButton.setBounds(infoRegion.withTrimmedBottom(80).removeFromBottom(80));

        // top right corner
        aviaryLogo.setBounds(getLocalBounds().reduced(20).removeFromTop(aviaryLogo.getHeight()).removeFromRight(aviaryLogo.getWidth()));
    }

private:
    std::unique_ptr<juce::Drawable> drawableLogoString = nullptr;

    Label versionLabel;
    Label descriptionLabel;

    std::unique_ptr<juce::FileChooser> presetFolderChooser;

    juce::Image vstLogo{juce::ImageCache::getFromMemory(BinaryData::vsticon_png, BinaryData::vsticon_pngSize)};
    juce::Image clapLogo{juce::ImageCache::getFromMemory(BinaryData::clapicon_png, BinaryData::clapicon_pngSize)};
    juce::Image gplLogo{juce::ImageCache::getFromMemory(BinaryData::gplv3icon_png, BinaryData::gplv3icon_pngSize)};
    juce::Image womp{juce::ImageCache::getFromMemory(BinaryData::wompwuzhere_png, BinaryData::wompwuzhere_pngSize)};

    juce::Image feature1{juce::ImageCache::getFromMemory(BinaryData::feature1_jpg, BinaryData::feature1_jpgSize)};
    juce::Image feature2{juce::ImageCache::getFromMemory(BinaryData::feature2_jpg, BinaryData::feature2_jpgSize)};
    juce::Image feature3{juce::ImageCache::getFromMemory(BinaryData::feature3_jpg, BinaryData::feature3_jpgSize)};
    juce::Image feature4{juce::ImageCache::getFromMemory(BinaryData::feature4_jpg, BinaryData::feature4_jpgSize)};

    juce::TextButton changePresetFolderButton;

    AviaryLogo aviaryLogo;

    HamburgerLAF hamburgerLAF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Info)
};