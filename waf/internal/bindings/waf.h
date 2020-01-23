//
//  PowerWAF
//  Copyright © 2019 Sqreen. All rights reserved.
//

#ifndef PowerWAF_h
#define PowerWAF_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_REGEX_STRING_LENGTH 4096

typedef enum
{
	PWI_INVALID          = 0,
	PWI_SIGNED_NUMBER    = 1 << 0,	// `value` shall be decoded as a int64_t (or int32_t on 32bits platforms)
	PWI_UNSIGNED_NUMBER  = 1 << 1,	// `value` shall be decoded as a uint64_t (or uint32_t on 32bits platforms)
	PWI_STRING           = 1 << 2,	// `value` shall be decoded as a UTF-8 string of length `nbEntries`
	PWI_ARRAY            = 1 << 3,	// `value` shall be decoded as an array of PWArgs of length `nbEntries`, each item having no `parameterName`
	PWI_MAP              = 1 << 4,	// `value` shall be decoded as an array of PWArgs of length `nbEntries`, each item having a `parameterName`
} PW_INPUT_TYPE;

typedef struct
{
	const char * parameterName;
	uint64_t parameterNameLength;
	const void * value;
	uint64_t nbEntries;
	PW_INPUT_TYPE type;
} PWArgs;

/// InitializePowerWAF
///
/// Initialize a rule in the PowerWAF
/// Must be called before calling RunPowerWAF on this rule name
/// Will clear any existing rule with the same name
///
/// @param ruleName Name the atom that provided the patterns we're about to initialize with
/// @param wafRule JSON blob containing the patterns to work with
/// @return The success (true) or faillure (false) of the init

extern bool powerwaf_initializePowerWAF(const char * ruleName, const char * wafRule);


typedef enum
{
	PWD_PARSING_JSON = 0,
	PWD_PARSING_RULE,
	PWD_PARSING_RULE_FILTER,
	PWD_OPERATOR_VALUE,
	PWD_DUPLICATE_RULE,
	PWD_PARSING_FLOW,
	PWD_PARSING_FLOW_STEP,
	PWD_MEANINGLESS_STEP,
	PWD_DUPLICATE_FLOW,
	PWD_DUPLICATE_FLOW_STEP,
} PW_DIAG_CODE;

/// powerwaf_initializePowerWAFWithDiag
///
/// Initialize a rule in the PowerWAF
/// Must be called before calling RunPowerWAF on this rule name
/// Will clear any existing rule with the same name
/// If any error is encountered and an errors pointer is provided, it'll be populated with a report in JSON
///
/// @param ruleName Name the atom that provided the patterns we're about to initialize with
/// @param wafRule JSON blob containing the patterns to work with
/// @param errors Pointer to the pointer to be populated with a potential error report. Set to NULL not to generate such a report
/// @return The success (true) or faillure (false) of the init


extern bool powerwaf_initializePowerWAFWithDiag(const char * ruleName, const char * wafRule, char ** errors);

/// powerwaf_freeDiagnotics
///
/// Free the error report generated by powerwaf_initializePowerWAFWithDiag
///
/// @param errors Pointer to a populated error report. NULL will be safely ignored


extern void powerwaf_freeDiagnotics(char * errors);

/// ClearRule
///
///	Flush all context related to a rule
///
/// @param ruleName Name of the rule to unload

extern void powerwaf_clearRule(const char * ruleName);

/// ClearAll
///
///	Flush all context

extern void powerwaf_clearAll(void);


typedef enum
{
	PW_ERR_INTERNAL     = -6,
	PW_ERR_TIMEOUT      = -5,
	PW_ERR_INVALID_CALL = -4,
	PW_ERR_INVALID_RULE = -3,
	PW_ERR_INVALID_FLOW = -2,
	PW_ERR_NORULE       = -1,
	PW_GOOD             =  0,
	PW_MONITOR          =  1,
	PW_BLOCK            =  2
} PW_RET_CODE;

typedef struct
{
	PW_RET_CODE action;
	const char * data;
} PWRet;

/// RunPowerWAF
///
/// Run the patterns from a rule on a set of parameters
///
/// Threading guarantees: When calling this API, a lock will be taken for a very short window as this call will take ownership of a shared smart pointer.
/// 	This pointer implement reference counting and can be owned by as many thread as you want.
/// 	If you call powerwaf_initializePowerWAF while evaluation of powerwaf_runPowerWAF is ongoing, the calls having already taken ownership will safely finish processing.
/// 	The shared pointer will be destroyed, without locking powerwaf_initializePowerWAF, when the last powerwaf_runPowerWAF finish processing.
///
/// Maximum budget: The budget is internally stored in nanoseconds in an int64_t variable. This is then added to the current time, also coded in nano seconds.
/// 	Due to those convertions, the maximum safe value for the next 15 years is 2^52. After that, 2^51.
///
/// @param ruleName Name of the rule you want to run
/// @param parameters The request's parameters
/// @param timeLeftInUs The maximum time in microsecond PowerWAF is allowed to take
/// @return Whether the pattern matched or whether we encountered an error

extern PWRet * powerwaf_runPowerWAF(const char * ruleName, const PWArgs * parameters, size_t timeLeftInUs);


typedef struct {
	uint16_t major;
	uint16_t minor;
	uint16_t patch;
} PWVersion;

/// GetVersion
///
/// Return the API version of PowerWAF
///
/// @return The API version in SemVer form

extern PWVersion powerwaf_getVersion(void);


typedef enum
{
	PWL_TRACE,
	PWL_DEBUG,
	PWL_INFO,
	PWL_WARN,
	PWL_ERROR,
	
	_PWL_AFTER_LAST,
} PW_LOG_LEVEL;

///
/// Callback that powerwaf will call to relay messages to the binding.
///
/// @param level The logging level
/// @param function The native function that emitted the message. Never NULL
/// @param file The file of the native function that emmitted the message. Never null
/// @param line The line where the message was emmitted. Non-negative
/// @param message The size of the logging message. NUL-terminated
/// @param message_len The length of the logging message (excluding NUL terminator)
///

typedef void (*powerwaf_logging_cb_t)(
		PW_LOG_LEVEL level, const char *function, const char *file, int line,
		const char *message, size_t message_len);

///
/// Sets up PowerWAF to rely logging messages to the binding
///
/// @param cb The callback to call, or NULL to stop relaying messages
/// @param min_level The minimum logging level for which to relay messages (ignored if cb is NULL)
/// @return whether the logging sink was successfully replaced
///
bool powerwaf_setupLogging(powerwaf_logging_cb_t cb, PW_LOG_LEVEL min_level);

/// PWArgs utils

extern PWArgs powerwaf_getInvalidPWArgs(void);
extern PWArgs powerwaf_createStringWithLength(const char * string, size_t length);
extern PWArgs powerwaf_createString(const char * string);
extern PWArgs powerwaf_createInt(int64_t value);
extern PWArgs powerwaf_createUint(uint64_t value);
extern PWArgs powerwaf_createArray(void);
extern PWArgs powerwaf_createMap(void);
extern bool powerwaf_addToPWArgsArray(PWArgs * array, PWArgs entry);
// Setting entryNameLength to 0 will result in the entryName length being re-computed with strlen
extern bool powerwaf_addToPWArgsMap(PWArgs * map, const char * entryName, size_t entryNameLength, PWArgs entry);
extern void powerwaf_freeInput(PWArgs *input, bool freeSelf);
extern void powerwaf_freeReturn(PWRet *output);

#ifdef __cplusplus
}
#ifdef TESTING
extern std::unordered_map<std::string, std::shared_ptr<PowerWAF>> & exportInternalRuleCollection();
#endif

#endif /* __cplusplus */

#endif /* PowerWAF_h */