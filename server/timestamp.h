#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_
#include <string>

class Timestamp 
{
public:
	Timestamp() 
		:micro_seconds_since_epoch_(0)
	{}
	explicit Timestamp(int micor_second) 
		:micro_seconds_since_epoch_(micor_second)
	{
	}

	std::string toString() const;

	static Timestamp now();
	static Timestamp invalid()
	{
		return Timestamp();
	}

	int64_t microSecondsSinceEpoch() const { return micro_seconds_since_epoch_; }
	time_t secondsSinceEpoch() const
	{
		return static_cast<time_t>(micro_seconds_since_epoch_ / kMicroSecondsPerSecond);
	}

	static Timestamp fromUnixTime(time_t t)
	{
		return fromUnixTime(t, 0);
	}

	static Timestamp fromUnixTime(time_t t, int microseconds)
	{
		return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
	}

	static const int kMicroSecondsPerSecond = 1000 * 1000;
private:
	int64_t micro_seconds_since_epoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
	return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
	return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline double timeDifference(Timestamp high, Timestamp low)
{
	int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
	return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds)
{
	int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
	return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}


#endif
