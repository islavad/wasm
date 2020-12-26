#include "../cube_api.h"
#include <cstdio>

class CEventLoopEx: public CEventLoop
{
    int m_nPos = 0;
    uint32_t m_nPrevTime = 0;
    uint8_t m_myCID = 0;
    Get_TRBL_1_0 m_trbl = {};
    Get_Gyro_1_0 m_gyro = {};
    Get_Accel_1_0 m_accel = {};
    bool m_bRelayRace = false;

protected:
    virtual void OnTRBLChanged(const Get_TRBL_1_0& trbl)
    {
        m_trbl = trbl;
    }

    virtual void OnGyroChanged(const Get_Gyro_1_0& gyro)
    {
        m_gyro = gyro;
        //NativePrint("OnGyroChanged X:%f Y:%f Z:%f", gyro.axis_X, gyro.axis_Y, gyro.axis_Z);
    }

    virtual void OnAccelChanged(const Get_Accel_1_0& accel)
    {
        m_accel = accel;
        //NativePrint("OnAccelChanged X:%f Y:%f Z:%f", accel.axis_X, accel.axis_Y, accel.axis_Z);
    }

    virtual bool OnTick(uint32_t time)
    {
        for (int display = 0; display < 3; ++display)
        {
            CDisplay disp(display);
            disp.Fill(fColor(1,1,1));

            if (m_bRelayRace)
            {
                disp.FillCircle(120,120, 60, 100, 2);
            }
        }
        return CEventLoop::OnTick(time);
    }

    virtual void OnMessage(uint32_t size, const Get_Message_1_0& msg)
    {
        static char race[] = "race!";
        static char race_ok[] = "race_ok";

        if (!size) {//comes only once at start to set own CID
            m_myCID = msg.from_cid;
            if (m_myCID == 0) { //starting the relay race!
                m_bRelayRace = true;
                NativeSend(1, race);
            }
        } else {
            if (size == sizeof(race) && !strcmp(race, (const char*)msg.data)) {
                m_bRelayRace = true;
                NativeSend(msg.from_cid, race_ok); //confirming we took it
                NativeSend((m_myCID + 1) % 8, race); //send race to another
            } else {
                if (size == sizeof(race_ok) && !strcmp(race_ok, (const char*)msg.data))
                {
                    m_bRelayRace = false;
                }
            }
        }
    }

public:
    virtual int Main()
    {
        NativePrint("Hello WOWd\n");
        NativeInvoke( Send_Message_1_0{ estSelf, 0, NULL} ); //to get own CID, search for m_myCID to see how it is set
        return CEventLoop::Main();
    }
};

WASM_EXPORT int run() // native cube code searches for this function and runs as a main()
{
    //whatever you return here will just be recorded into logs
    return CEventLoopEx().Main();
}