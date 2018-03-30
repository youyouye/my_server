#include "timestamp.h"
#include <inttypes.h>
#include <sys/time.h>

std::string Timestamp::toString() const
{
	char buf[32] = {0};
	int64_t seconds = micro_seconds_since_epoch_ / kMicroSecondsPerSecond;
	int64_t microseconds = micro_seconds_since_epoch_ % kMicroSecondsPerSecond;
	snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
	return buf;
}

Timestamp Timestamp::now()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t seconds = tv.tv_sec;
	return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}








