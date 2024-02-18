#pragma once

#include <CommandLineServer.h>

/**
* Uptime command, prints the uptime (response of millis()) in days, hours, minutes and seconds.
*/
struct UptimeCommand : public ICommandLineHandler {
	virtual bool execute(Print& output) override {
		uint32_t ms = millis();

		constexpr uint32_t MS_SECOND = 1000u;
		constexpr uint32_t MS_MINUTE = MS_SECOND * 60;
		constexpr uint32_t MS_HOUR = MS_MINUTE * 60;
		constexpr uint32_t MS_DAY = MS_HOUR * 24;

		uint32_t days = ms / MS_DAY;
		uint32_t hours = (ms / MS_HOUR) % 24;
		uint32_t minutes = (ms / MS_MINUTE) % 60;
		uint32_t seconds = (ms / MS_SECOND) % 60;

		output.printf("Uptime: %u days, %02u:%02u:%02u\n", days, hours, minutes, seconds);
		return true;
	}
};
