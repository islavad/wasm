#include "cube_api.h"
#include <cstdio>
#include "Resources/happy.bmp.h"

template<uint16_t _size, class T>
class CCubeNet
{
public:
    static const uint16_t width = _size * 4;
    static const uint16_t height = _size * 3;
    static const uint16_t size = _size;

    T& At(int16_t x, int16_t y)
    {
        x %= width;
        y %= height;
        int16_t nx = 0;
        int16_t ny = 0;
        if (size < y < 2*size || size < x < 2*size) // direct mapping on the cross
        {
            nx = x;
            ny = y;
        } else if (x < size && y < size) {// leftmost upper quadrant
            nx = size*2 - y;
            ny = x;
        }
        return m_values[ny][nx];
    }

protected:
    T m_values[height][width];
};

const int g = 10;
const int k = 240 / g;
char b[k*k] = {};

const uint16_t pixel = 10;
CCubeNet<240*2/pixel, bool> g_cube;


class CEventLoopEx: public CEventLoop
{
    int m_nPos = 0;
    uint32_t m_nPrevTime = 0;
    uint8_t m_myCID = 0;
    Get_TRBL_1_0 m_trbl = {};

    size_t print_transpon(const uint8_t(&matrix)[8][3], char* buffer) {
        uint8_t result[3][8];

        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 3; j++)
                result[j][i] = matrix[i][j];

        int l = 0;
        for (int j = 0; j < 3; j++) {
            if (j)
                buffer[l++] = '\n';
            for (int i = 0; i < 8; i++) {
                if (i)
                    buffer[l++] = ' ';
                l += sprintf(&buffer[l], "%x", result[j][i]);
            }
        }
        buffer[l++] = 0;
        return l;
    }

    Get_Gyro_1_0 m_gyro = {};
    Get_Accel_1_0 m_accel = {};
protected:
    virtual void OnTRBLChanged(const Get_TRBL_1_0& trbl)
    {
        m_trbl = trbl;
        char buffer[256];
        print_transpon(m_trbl.CID, buffer);
        NativePrint("OnTRBLChanged CID %s", buffer);

        print_transpon(m_trbl.CFID, buffer);
        NativePrint("OnTRBLChanged CFID %s", buffer);

        print_transpon(m_trbl.CFMID, buffer);
        NativePrint("OnTRBLChanged CFMID %s", buffer);
    }
    virtual void OnGyroChanged(const Get_Gyro_1_0& gyro)
    {
        m_gyro = gyro;
        NativePrint("OnGyroChanged X:%x Y:%x Z:%x", gyro.axis_X, gyro.axis_Y, gyro.axis_Z);
    }
    virtual void OnAccelChanged(const Get_Accel_1_0& accel)
    {
        m_accel = accel;
        NativePrint("OnAccelChanged X:%x Y:%x Z:%x", accel.axis_X, accel.axis_Y, accel.axis_Z);
    }

    virtual bool OnTick(uint32_t time)
    {
        for (int display = 0; display < 3; ++display)
        {
            CDisplay disp(display);
            disp.Fill(fColor(1,1,1));

            if (display == 0)
                Update(disp);

            if (display == 1)
            {
                for (uint16_t y = 0; y < g_cube.height; ++y)
                    for (uint16_t x = 0; x < g_cube.width; ++x)
                        disp.FillRect(x * pixel, y * pixel, pixel, pixel, g_cube.At(x, y) ? fColor(1,1,1) : fColor(0,0,0));
                ++m_nPos %= 240;
                disp.DrawText(120, 120, "Life!", fColor(1,1,1), 30, m_nPos % 360);
            }

            if (display == 2)
            {
                disp.DrawLine(0,0,240,240, 100);
                disp.FillRect(m_nPos, m_nPos, 240, 240, fColor(0,1,0));
                disp.DrawPixelAlpha(66, 66, 255, 2);
                //disp.FillCircle(120,120, 30, 100, 2);
                if (display == 2) {
                    CBitmap b;
                    if (b.Load(happy_bmp, happy_bmp_len, (int)EPictureFormat::epfRGB565)) {
                        NativePrint("TRY DRAW BITMAP %d", b.GetSize());
                        disp.DrawBitmap(0, 0, b, 1, m_nPos, 0);
                    }
                }
            }

            if (m_nPrevTime)
            {
                uint32_t diff = time - m_nPrevTime;
                static char buff[20] = {};
                sprintf(buff, "fps: %f", 1000. / diff);
                disp.DrawText(0, 0, buff, fColor(1,1,1), 30);
            }
            m_nPrevTime = time;

            //NativePrint("Draw for display %d, time: %d\n", display, time);
        }
        return CEventLoop::OnTick(time);
    }

    void Draw(CDisplay& disp, int x, int y, int w, int h, bool fill)
    {
        disp.FillRect(x, y, w, h, fill ? fColor(1,0,0) : fColor(0,0,1));
    }

    void Randomize()
    {
        for (int y = k * k; y--;)
            b[y] = rand() % 2;

        for (uint16_t y = 0; y < g_cube.height; ++y)
            for (uint16_t x = 0; x < g_cube.width; ++x)
                g_cube.At(x, y) = !!(rand() % 2);
    }

    void Update(CDisplay& disp) {
        char c[k*k] = {};
        for (int y = k * k; y--;) {
            int n = 0;
            for (int f = 9; f--;) //going around 
                n += b[(y / k + k + f % 3 - 1) % k * k + (y + k + f / 3 - 1) % k];
            c[y] = (n == 3) || (n - b[y] == 3);
            Draw(disp, y % k * g, (y / k) * g, g - 1, g - 1, c[y]);
        }

        if (0 != memcmp(b, c, sizeof(b)))
            memcpy(b, c, sizeof(b));
        else
            Randomize();
    }

    virtual void OnMessage(const Get_Message_1_0& msg)
    {

        if (!msg.data) {
            m_myCID = msg.from_cid;
        }

        NativePrint("Msg from %d: %s", msg.from_cid, (const char*)msg.data);
        char answer[] = "I am fine too!";
        NativeInvoke(Send_Message_1_0{msg.from_cid, answer, sizeof(answer)});
    }

public:
    virtual int Main()
    {
        NativePrint("Hello WOWd\n");
        Randomize();
        NativeInvoke( Send_Message_1_0{ estSelf, NULL, 0 } );
        return CEventLoop::Main();
    }
};

#include "rubic/header.h"

extern "C" int pawn_run(cell* pkt, int size, int* src);

bool TASK_pawnScriptRunner()
{
    


    cell amx_pkt_tick[] = {CMD_TICK, 0, 0, 0};
    cell amx_pkt_geo[20] = {0};
    cell amx_pkt_mtd[16] = {0};

    pawn_run(amx_pkt_geo, sizeof(amx_pkt_geo)/sizeof(cell), 0);
    pawn_run((cell*)amx_pkt_mtd, sizeof(amx_pkt_mtd)/sizeof(cell), 0);

    for (int i = 0; i < 1000; ++i)
        pawn_run((cell*)amx_pkt_tick, sizeof(amx_pkt_tick)/sizeof(cell), 0);

    return true;
}


WASM_EXPORT int run() // native cube code searches for this function and runs as a main()
{
    TASK_pawnScriptRunner();
    //whatever you return here will just be recorded into logs
    return CEventLoopEx().Main();
}


WC_EXTERN_C int sendpacket(int* packet, int size)
{
    return 0;
}