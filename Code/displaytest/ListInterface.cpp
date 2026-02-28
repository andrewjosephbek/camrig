#include "ListInterface.h"
#include <cstdint>
#include <cstring>
#include <U8g2lib.h>

const uint8_t OLED_128x64_ListInterface::lineYPos[4] = { 19, 36, 49, 62 };

const uint8_t* OLED_128x64_ListInterface::lineFonts[4] = {
  u8g2_font_6x10_tf,
  u8g2_font_t0_17_tf,
  u8g2_font_6x10_tf,
  u8g2_font_6x10_tf 
};

OLED_128x64_ListInterface::OLED_128x64_ListInterface(U8G2_SH1106_128X64_NONAME_F_HW_I2C* u8g2_display,
                const uint8_t nListElements, 
                const uint8_t nUIElements, 
                char** staticTextLines, 
                int** dynamicParamsLines,
                char* supTitle)

:  u8g2_display(u8g2_display), 
    nListElements(nListElements), 
    nUIElements(nUIElements), 
    dynamicLineXPos(90), 
    staticLineXPos(0),
    uiState(1 << 31),
    listIdx(0)
{
                 
    size_t titleLength = std::strlen(supTitle);
    this->supTitle = new char[titleLength + 1];
    std::strcpy(this->supTitle, supTitle);

    this->staticTextLines = new char*[nListElements];
    for (uint8_t i = 0; i < nListElements; ++i) {
        size_t len = std::strlen(staticTextLines[i]);
        this->staticTextLines[i] = new char[len + 1];
        std::strcpy(this->staticTextLines[i], staticTextLines[i]);
    }

    this->dynamicParamsLines = nullptr;

    if (dynamicParamsLines != nullptr) {
        this->dynamicParamsLines = new int*[nListElements];
        for (uint8_t i = 0; i < nListElements; ++i) {
            this->dynamicParamsLines[i] = dynamicParamsLines[i];
        }
    }
}

OLED_128x64_ListInterface::~OLED_128x64_ListInterface() {
    delete[] lineYPos;

    delete[] lineFonts; 

    for (uint8_t i = 0; i < nListElements; ++i) {
        delete[] staticTextLines[i];
    }
    
    delete[] staticTextLines;

    if(dynamicParamsLines != nullptr) 
        delete[] dynamicParamsLines;  

    delete[] supTitle; 
}

void OLED_128x64_ListInterface::drawList(int8_t scrollDir, bool uiScrollMode) {
    u8g2_display->clearBuffer();

    // Draw header
    u8g2_display->setFont(u8g2_font_tinytim_tf);
    u8g2_display->drawStr(0, 5, this->supTitle);
    u8g2_display->drawLine(0, 6, 125, 6);

    // If in scrolling mode, update the UI with the new scroll postion
    // If list does not have dynamic elements, always stay in scroll mode
    if (uiScrollMode)
        uiState += scrollDir;

    char buffer[5];

    // Draw each element of the UI line by line
    for (int i = 0; i < nUIElements; i++) {
        // convert the line number (i) to the indices of the elements to be displayed (j)
        // conversion is neccessary when list contains more elements than can be displayed
        listIdx = wrappedIndex(uiState - i, nListElements);
        int y_pos = lineYPos[i];

        // Draw static values (text/title on each line)
        u8g2_display->setFont(lineFonts[i]);
        u8g2_display->drawStr(staticLineXPos, y_pos, staticTextLines[listIdx]);

        // If list does not have dynamic lines, don't draw them
        if(dynamicParamsLines == nullptr)
            continue;

        // Only edit when not scrolling, editing is always on the 2nd line
        if (!uiScrollMode && i == 1) {
            *dynamicParamsLines[listIdx] += 10 * scrollDir;

            u8g2_display->setFont(u8g2_font_m2icon_5_tf);
            u8g2_display->drawStr(120, 5, "N");
        }

        // Draw dynamic value (parameters values)
        u8g2_display->setFont(lineFonts[i]);
        snprintf(buffer, sizeof(buffer), "%d", *dynamicParamsLines[listIdx]);
        u8g2_display->drawStr(dynamicLineXPos, y_pos, buffer);
    }

    u8g2_display->sendBuffer();
}