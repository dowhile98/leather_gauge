/*
 * dmp.h
 *
 *  Created on: Aug 19, 2024
 *      Author: eplim
 */

#ifndef COMPONENTS_DMD_DMD_H_
#define COMPONENTS_DMD_DMD_H_

/*Includes ---------------------------------------------------------------------*/
#include "Bitmap.h"
#include "dmd_config.h"
#include <cstddef>

/*Defines ----------------------------------------------------------------------*/
#define CONNECT_NORMAL 0
#define CONNECT_ZIGZAG 1
/*Class definition -------------------------------------------------------------*/
class DMDESP : public Bitmap
{
public:
    explicit DMDESP(int widthPanels = 1, int heightPanels = 1);
    ~DMDESP();

    bool IsUseDoubleBuffer() const { return useDoubleBuffer; }
    void setDoubleBuffer(bool state);
    void swapBuffers();
    void swapBuffersAndCopy();

    void start();
    void refresh();
    void loop();

    void setBrightness(uint8_t brightness);
    void updateFlag(void);

    bool mutexTake(BaseType_t wait = portMAX_DELAY);
    void mutexRelease(void);

    void setConnectScheme(uint8_t sch);

    void clearBuffer(void);
private:
    // Disable copy constructor and operator=().
    DMDESP(const DMDESP &other) : Bitmap(other) {}
    DMDESP &operator=(const DMDESP &) { return *this; }

    uint16_t brightness;
    bool useDoubleBuffer;
    uint8_t phase;
    uint8_t *fb0;
    uint8_t *fb1;
    uint8_t *displayfb;
    uint64_t lastRefresh;
    SemaphoreHandle_t tickFlag;
    SemaphoreHandle_t mutex;
    uint8_t connectScheme;
};

#endif /* COMPONENTS_DMD_DMD_H_ */
