#include "ProcCity/Environment/Utils/BlockEnvironmentUtils.h"

namespace
{
	struct FWidthSpecEntry
	{
		float WidthValue;
		EBlockPathWidthSpec Spec;
	};
		
	const FWidthSpecEntry Entries[] =
	{
		{150.0f, EBlockPathWidthSpec::W150},
		{200.0f, EBlockPathWidthSpec::W200},
		{300.0f, EBlockPathWidthSpec::W300},
		{400.0f, EBlockPathWidthSpec::W400},
		{600.0f, EBlockPathWidthSpec::W600},
		{800.0f, EBlockPathWidthSpec::W800},
		{1000.0f, EBlockPathWidthSpec::W1000},
		{1200.0f, EBlockPathWidthSpec::W1200},
		{1600.0f, EBlockPathWidthSpec::W1600}
	};
}

namespace ProcCity
{
	bool TryConvertWidthToPathWidthSpec(
		const float Width,
		EBlockPathWidthSpec& OutSpec,
		const float Tolerance
	)
	{
		for (const FWidthSpecEntry& Entry : Entries)
		{
			if (FMath::Abs(Width - Entry.WidthValue) <= Tolerance)
			{
				OutSpec = Entry.Spec;
				return true;
			}
		}
		
		OutSpec = EBlockPathWidthSpec::None;
		return false;
	}
	
	bool TryGetPathWidthFromWidthSpec(
		const EBlockPathWidthSpec Spec,
		float& OutWidth
	)
	{
		for (const FWidthSpecEntry& Entry : Entries)
		{
			if (Spec == Entry.Spec)
			{
				OutWidth = Entry.WidthValue;
				return true;
			}
		}
		
		OutWidth = 0.0f;
		return false;
	}
}