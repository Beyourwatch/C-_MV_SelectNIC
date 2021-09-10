#include <apps/Common/exampleHelper.h>
#include <functional>
#include <iostream>
#include <mvIMPACT_CPP/mvIMPACT_acquire_helper.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>
#ifdef _WIN32
#   include <mvDisplay/Include/mvIMPACT_acquire_display.h>
using namespace mvIMPACT::acquire::display;
#   define USE_DISPLAY
#endif // #ifdef _WIN32



using namespace mvIMPACT::acquire;
using namespace std;
DeviceManager devMgr;


//=============================================================================
//================= Data type definitions =====================================
//=============================================================================
//-----------------------------------------------------------------------------
struct ThreadParameter
//-----------------------------------------------------------------------------
{
    Device* pDev_;
    unsigned int requestsCaptured_;
    Statistics statistics_;
#ifdef USE_DISPLAY
    ImageDisplayWindow displayWindow_;
#endif // #ifdef USE_DISPLAY
    explicit ThreadParameter( Device* pDev ) : pDev_( pDev ), requestsCaptured_( 0 ), statistics_( pDev )
#ifdef USE_DISPLAY
        // initialise display window
        // IMPORTANT: It's NOT safe to create multiple display windows in multiple threads!!!
        , displayWindow_( "mvIMPACT_acquire sample, Device " + pDev_->serial.read() )
#endif // #ifdef USE_DISPLAY
    {}
    ThreadParameter( const ThreadParameter& src ) = delete;
    ThreadParameter& operator=( const ThreadParameter& rhs ) = delete;
};

//=============================================================================
//================= implementation ============================================
//=============================================================================
//-----------------------------------------------------------------------------
void myThreadCallback(shared_ptr<Request> pRequest, ThreadParameter& threadParameter)
//-----------------------------------------------------------------------------
{
	++threadParameter.requestsCaptured_;
	// display frameID

	if (pRequest->isOK())
	{
		cout<< pRequest->infoFrameID << endl;
	}
	else
	{
		cout << "Error: " << pRequest->requestResult.readS() << endl;
	}
	
}

//-----------------------------------------------------------------------------
int main( void )
//-----------------------------------------------------------------------------
{
    Device* pDev = getDeviceFromUserInput(devMgr);
    if( pDev == nullptr )
    {
        cout << "Unable to continue! Press [ENTER] to end the application" << endl;
        cin.get();
        return 1;
    }

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 通过句柄寻找设备的interface参数
	PropertyI64 interfaceID;
	DeviceComponentLocator locator(pDev->hDev());
	locator.bindComponent(interfaceID, "InterfaceID");
	
	// 显示目前打开设备的网口
	cout << "Current connected Interface:" << interfaceID <<  endl;
	
	// 枚举所有设备能到达的网口
	vector<string> interfaceIDstring;
	PropertyI64 interfaceIDtemp = interfaceID.getTranslationDictStrings(interfaceIDstring);

	cout << "Found camera connected to the interface: " << endl;

	for (int i = 0; i < interfaceIDstring.size(); i++)
	{
		cout << "index[" << i << "]:" << interfaceIDstring[i] << endl;
	}

	// 输入需要的index
	unsigned int interfaceNr = 0;
	std::cin >> interfaceNr;
	// remove the '\n' from the stream
	std::cin.get();

	// 写入连接需要的interface
	interfaceID.writeS(interfaceIDstring[interfaceNr]);
		
	cout << "Current connected Interface:" << interfaceID << endl;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////

    cout << "Initialising the device. This might take some time..." << endl;
    try
    {
        pDev->open();
    }
    catch( const ImpactAcquireException& e )
    {
        // this e.g. might happen if the same device is already opened in another process...
        cout << "An error occurred while opening the device " << pDev->serial.read()
             << "(error code: " << e.getErrorCodeAsString() << ").";
        return 1;
    }

	//验证，输出连接设备的 application IP
	mvIMPACT::acquire::GenICam::TransportLayerControl tlControl(pDev);
	cout << tlControl.gevPrimaryApplicationIPAddress.readS()<< endl ;
		


    ThreadParameter threadParam( pDev );
    helper::RequestProvider requestProvider( pDev );
    requestProvider.acquisitionStart( myThreadCallback, std::ref(threadParam));
    cin.get();
    requestProvider.acquisitionStop();
    return 0;
}
