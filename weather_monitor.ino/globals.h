
// Timing
const int RECONNECT_DELAY = 10;
const unsigned int UPDATE_DELAY = 5 * 60 * 1000; // 5 min
const unsigned int UPDATE_TIME_DELAY = 1000;

// HTTP Codes
const int HTTP_SUCCESS = 200;

// URL
const String URL_GET_LOCATION = "http://ip-api.com/json/";
const String URL_GET_WEATHER_DATA = "https://api.open-meteo.com/v1/forecast?latitude=$lat$&longitude=$lon$&hourly=temperature_2m,precipitation_probability,weather_code&forecast_days=2";
const String URL_GET_HOUR_OFFSET = "http://api.geonames.org/timezoneJSON?lat=$lat$&lng=$lon$&username=$username$";