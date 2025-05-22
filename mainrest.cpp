//#include <windows.h>
#include <wincrypt.h>


#pragma comment(lib, "crypt32.lib")

extern deviceRestService iDeviceRestService;
secuGenCard secugenCard;
HidDevice   hidDeviceObj;//added
CertGen cg;
bool methodValidation(std::string methodString, string methodName) {

	if (methodName == "GET") {
		if (methodString == "/Device/Client/card/list" ")
			return true;
	}
	return false;
}

void get_method_handler(const shared_ptr< Session > session)
{

	logging::INFO("Rest service : get_method_handler Enter");
	const auto request = session->get_request();
	auto authorisation = request->get_header("Authorization");

	logging::INFO("Rest service authorization cod for test" + authorisation);
#ifdef IS_SSLENABLE
	/*if (authorisation not_eq "Basic RXBpY0lkZW50aXR5OkVwaWNJZGVudGl0eUAxMjM0")
	{
		session->close(FORBIDDEN, { { "WWW-Authenticate", "Basic realm=\"restbed\"" } });
		return;
	}*/
	if (authorisation not_eq "Basic RXBpY0lkZW50aXR5OkVwaWNJZGVudGl0eUAxMjM0")
	{
		session->add_header("Access - Control - Allow - Origin", " * ");
		session->close(FORBIDDEN, { { "WWW-Authenticate", "Basic realm=\"restbed\"" } });
	}
#endif

	std::string responseString = "";
	char responsesize[8];

	cout << request->get_path() << endl;

	if (request->get_path() == "/Device/Client/card/list")
	{

		logging::INFO("Rest service : /Device/Client/card/list");
		DeviceList deviclist;
		walletInfo wallet;
		LONG rv = secugenCard.getCardList(wallet, L"");
		deviclist.mWalletInfo = &wallet;
		jsonParserCreater jsonbuilder(deviclist.AsJSON());
		responseString = jsonbuilder.getJsonString();
	}
	else if (request->get_path() == "/Device/Client/card/status")
	{
		string cardReaderName = request->get_query_parameter("cardReaderName");
		logging::INFO("Rest service : /Device/Client/card/status");
		//DeviceList deviclist;
		responseString = "";
		try {

			walletInfo wallet;
			string atr = "", responseCode = "";
			if (!cardReaderName.empty()) {
				string_t cardReaderNameWstr(cardReaderName.begin(), cardReaderName.end());
				LONG rv = secugenCard.getCardList(wallet, cardReaderNameWstr);
				//deviclist.mWalletInfo = &wallet;
				std::list<cardReaderInfo> cardWallet = wallet.getWallet();

				if (cardWallet.size() > 0) {
					cardReaderInfo ci = cardWallet.front();
					atr = ci.atr;
					if (atr != "")
						responseCode = "DS0000";
					else
						responseCode = "DS0033";//ATR not found.
				}
				else {
					responseCode = "DS0033";//ATR not found.
				}
			}
			else
				responseCode = "DS0133";//CARD Name Not Found f

			responseString = "{\"atr\":\"" + atr + "\" , \"responseCode\":\"" + responseCode + "\"}";
			//jsonParserCreater jsonbuilder(deviclist.AsJSON());
			//responseString = jsonbuilder.getJsonString();
		}
		catch (...) {
			logging::LOGERROR("Failed to get Card list. Got Exception.");
		}
	}

	else if (request->get_path() == "/Device/Client/health") {
		if (iDeviceRestService.isUp())
			responseString = "Looks Good";
		else
			responseString = "Not Good";

	}
	//aded


	else if (request->get_path() == "/Device/Client/type") {

		responseString = DEVICE_SERVICE_INSTALLER;

	}
	else if (request->get_path() == "/Device/Client/name") {

		responseString = DEVICE_SERVICE_NAME;

	}

	else if (request->get_path() == "/Device/Client/version") {

		string version = string("Gets the version of service the installer : ") + DEVICE_SERVICE_VERSION + DEVICE_SERVICE_INSTALLER;
		logging::INFO(version.c_str());
		responseString = DEVICE_SERVICE_VERSION;
	}
	//added system info
	else if (request->get_path() == "/device-client-info") {
		systemDevice data2;
		Info wall;
		systemData data;
		data.retrieveData(wall);
		data2.data = &wall;
		jsonParserCreater jsonbuilder(data2.AsJSON());
		responseString = jsonbuilder.getJsonString();
	}
	


deviceRestService::deviceRestService() {
	m_port = 9443;
}

deviceRestService::~deviceRestService() {

}

void deviceRestService::init() {

	try {
		// Build our listener's URI from the configured address and the hard-coded path "MyServer/Action"
		logging::INFO("Rest service is intitializing");

		std::set<std::string> paths;

		//Generic APIS
		paths.insert("/Device/Client/health");
		paths.insert("/Device/Client/version");
		paths.insert("/Device/Client/Type");
		paths.insert("/Device/Client/name");

		//Card related apis
		paths.insert("/Device/Client/card/list");
		
		
		
		settings->set_default_header("Access-Control-Allow-Origin", "*");
		settings->set_default_header("Access-Control-Allow-Headers", "*");//CORS 
		settings->set_default_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
		settings->set_default_header("Access-Control-Allow-Credentials", "true");
		settings->set_default_header("preflightContinue", "false");
		settings->set_port(1443);
#ifdef IS_SSLENABLE
		/*req.setHeader("Access-Control-Allow-Origin", "*");
		req.setHeader("Access-Control-Allow-Credentials", "true");
		req.setHeader("Access-Control-Allow-Methods", "ACL, CANCELUPLOAD, CHECKIN, CHECKOUT, COPY, DELETE, GET, HEAD, LOCK, MKCALENDAR, MKCOL, MOVE, OPTIONS, POST, PROPFIND, PROPPATCH, PUT, REPORT, SEARCH, UNCHECKOUT, UNLOCK, UPDATE, VERSION-CONTROL");
		req.setHeader("Access-Control-Allow-Headers", "Overwrite, Destination, Content-Type, Depth, User-Agent, Translate, Range, Content-Range, Timeout, X-File-Size, X-Requested-With, If-Modified-Since, X-File-Name, Cache-Control, Location, Lock-Token, If");
		req.setHeader("Access-Control-Expose-Headers", "DAV, content-length, Allow");*/

		ssl_settings->set_http_disabled(true);
		ssl_settings->set_port(2443);
		std::string enVval = (std::string)getenv("DEVICE_CLIENT_HOME");
		if (!enVval.empty()) {
			logging::INFO("DEVICE_CLIENT_HOME path is not empty");
		}
		else {
			// Path doesn't exist
			logging::INFO("DEVICE_CLIENT_HOME path empty");
			std::cerr << "Path doesn't exist: " << enVval << std::endl;

		}
		printf("%s", enVval.c_str());
		std::replace(enVval.begin(), enVval.end(), '\\', '/');
		enVval.replace(2, 1, "//");
		//HERE generating localhost certificates and store in conf 
		cg.initCert();
		logging::INFO("Rest service : local cert location : " + enVval + "/conf/cert/device-client.key");
		logging::INFO("Rest service : local cert location : " + enVval + "/conf/cert/device-client.crt");
		ssl_settings->set_private_key(enVval + "/conf/cert/device-client.key");
		ssl_settings->set_certificate(enVval + "/conf/cert/device-client.crt");
		ssl_settings->set_single_diffie_hellman_use_enabled(false);

		importpfxCertificate("localhost", enVval + "/conf/cert/rootCA.p12");

		settings->set_ssl_settings(ssl_settings);


#endif IS_SSLENABLE
		//added

		service.publish(resource);

		logging::INFO("Rest service is initialzed");

	}
	catch (exception ex) {
		printf(ex.what());
		logging::LOGERROR(ex.what());
		logging::LOGERROR("Rest service initialization Failed. Got Exception.");
	}
}

void deviceRestService::setPort(unsigned short port) {
	m_port = port;
}

void deviceRestService::startService() {
	try {
		logging::INFO("Rest service is going to start at port 2443");
		service.start(settings);
		logging::INFO("Rest service is stoped at port 2443");
	}
	catch (exception ex) {
		printf("%s", ex.what());
		logging::INFO("Failed to start service at port 2443 " + (string)ex.what());
	}
}

void deviceRestService::stopService() {

	service.stop();
}

bool deviceRestService::isUp() {
	return service.is_up();
}
