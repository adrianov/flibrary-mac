include_guard(GLOBAL)

# Файл для работы с именнованными ассоциативными массивами (словарями)
set(__MAP_PREFIX__ MAP_)
set(__EMPTY__ "_-=VERYYY_EMPTYYY=-_")
set(__multi_val_range)
foreach(__var_if RANGE 100)
	list(APPEND __multi_val_range VARIABLES_IF${__var_if} VARIABLES_IF${__var_if}ELSE APPEND_IF${__var_if} APPEND_IF${__var_if}ELSE)
endforeach()

########################################################
# Секция фукнций для работы с окруженеим (environment) #
########################################################

# Функция проверки существования перменной окружения
# Имя переменной окружения
# Существует или нет
function (ExistEnvironment _environmentNameVariable _isExist)
	if (DEFINED ENV{${_environmentNameVariable}})
		set(${_isExist} "true"  PARENT_SCOPE)
	else()
		set(${_isExist} "false"  PARENT_SCOPE)
	endif()
endfunction()

#---------------------------------------------------------------------------------------


# Функция добавления переменной исходя из переменной окружения
# Имя переменной
# Значение по умолчанию
function (AddVariable _environmentNameVariable _defaulValue)
	if (DEFINED ${_environmentNameVariable}.override)
		set (__local_override "${${_environmentNameVariable}.override}")
		if ("${__local_override}" STREQUAL "")
			set (${_environmentNameVariable} "${__EMPTY__}" PARENT_SCOPE)
		else()
			set (${_environmentNameVariable} "${__local_override}" PARENT_SCOPE)
		endif()
		return()
	endif()

	ExistEnvironment(${_environmentNameVariable} _isExistEnv)
	# Если переменная существет то создаем ее отображение
	if (_isExistEnv)
		set(_valueEnvVariable "$ENV{${_environmentNameVariable}}" )
		set (${_environmentNameVariable} "${_valueEnvVariable}" PARENT_SCOPE)
	else ()
		set (${_environmentNameVariable} "${_defaulValue}" PARENT_SCOPE)
	endif()
endfunction()

include(${CMAKE_CURRENT_LIST_DIR}/utils.cmake)

# Создаёт именованный словарь
# CreateMap(
#	<name>							# Имя словаря
#	[NO_CLEAR]				             # Не очищать словарь, оставляя уже существующие записи (подходит для нескольких CreateMap вызовов, создающих одну запись)
#	[INHERIT [<base_list>]]					# Базовые словаря
#	[INHERIT_WIN32 [<base_list>]]				# Базовые словаря только для WIN32
#	[INHERIT_APPLE [<base_list>]]				# Базовые словаря только для APPLE
#	[INHERIT_LINUX [<base_list>]]				# Базовые словаря только для LINUX
#	[INHERIT_ANDROID [<base_list>]]				# Базовые словаря только для ANDROID
#	[VARIABLES [<variable_definitions>]]			# Определяет безусловные переменные
#	[VARIABLES_WIN32 [<variable_definitions>]]		# Определяет переменные только для WIN32
#	[VARIABLES_APPLE [<variable_definitions>]]		# Определяет переменные только для APPLE
#	[VARIABLES_LINUX [<variable_definitions>]]		# Определяет переменные только для LINUX
#	[VARIABLES_ANDROID [<variable_definitions>]]		# Определяет переменные только для ANDROID
#	[VARIABLES_X86_64 [<variable_definitions>]]		# Определяет переменные только на x86_64 платформе
#	[VARIABLES_IFx <condition> [<variable_definitions>]]	# Определяет переменные которые будут добавлены в словарь при выполнении условия <condition>
#	[VARIABLES_IFxELSE [<variable_definitions>]]		# Определяет переменные которые будут добавлены в словарь при не выполненеии соответствующего IFx
#	[APPEND [<variable_definitions>]]			# Дополняет безусловные переменные
#	[APPEND_WIN32 [<variable_definitions>]]			# Дополняет переменные только для WIN32
#	[APPEND_APPLE [<variable_definitions>]]			# Дополняет переменные только для APPLE
#	[APPEND_LINUX [<variable_definitions>]]			# Дополняет переменные только для LINUX
#	[APPEND_ANDROID [<variable_definitions>]]		# Дополняет переменные только для ANDROID
#	[APPEND_IFx <condition> [<variable_definitions>]]	# Дополняет переменные которые будут добавлены в словарь при выполнении условия <condition>
#	[APPEND_IFxELSE [<variable_definitions>]]		# Дополняет переменные которые будут добавлены в словарь при не выполненеии соответствующего IFx
#	)
# base_list - списко базовых словарей, переменные из которых будут добавлены в создаваемый
# variable_definitions - список пар: variable_name value
#	value не может иметь пустое значение "", для задания пустого значения используется ${__EMPTY__}
# condition - применяется синтаксис аналогичный синтаксису if, за исключением:
# 	1) круглые скобки надо отделять пробелами с 2-х сторон. Например: "WIN32 AND ( ${PRODUCT_CONFIG} STREQUAL MovaviScreenCapture )"
# VARIABLES_IFx <condition> VARIABLES_IFxELSE - пары if(condition) else() endif(), где x {1..9}
function(CreateMap name)
	macro(CreateMap__ParseInherit base)
		set(base_prefix ${__MAP_PREFIX__}${base})
		string(LENGTH "${base_prefix}." base_prefix_length)
		set(__pairs)
		foreach(__base_var ${${base_prefix}.__LIST_VARIABLES})
			string(SUBSTRING "${__base_var}" ${base_prefix_length} -1 __new_var)
			list(APPEND __pairs ${__new_var} ${${__base_var}})
		endforeach(__base_var ${${base_prefix}.__LIST_VARIABLES})
		CreateMap__ParsePairs(false __pairs)
	endmacro(CreateMap__ParseInherit variables)

	macro(CreateMap__ParsePairs append variables)
		set(__lv ${${var_prefix}.__LIST_VARIABLES})
		CreatePairsKeyValue(${${variables}})
		foreach(var ${__CreatePairsKeyValue_keys})
			AddVariable(${var} ${${var}})
			if(${append} AND DEFINED ${var_prefix}.${var})
				set(__val "${${var_prefix}.${var}},${${var}}")
			else()
				set(__val ${${var}})
			endif()
			set(${var_prefix}.${var} ${__val} CACHE STRING "" FORCE)
			append_unique(__lv ${var_prefix}.${var})
			#message("${name} ${var_prefix}.${var} ${${var}}")
		endforeach(var ${__CreatePairsKeyValue_keys})
		set(${var_prefix}.__LIST_VARIABLES ${__lv} CACHE STRING "" FORCE)
		mark_as_advanced(${var_prefix}.__LIST_VARIABLES ${${var_prefix}.__LIST_VARIABLES})
	endmacro(CreateMap__ParsePairs append variables)

	macro(CreateMap__ClearCache)
		foreach(__var ${${var_prefix}.__LIST_VARIABLES})
			unset(${__var} CACHE)
			#message("Unset ${__var}")
		endforeach(__var ${${var_prefix}.__LIST_VARIABLES})
		unset(${var_prefix}.__LIST_VARIABLES CACHE)
	endmacro(CreateMap__ClearCache)

	set(__options NO_CLEAR)
	set(__one_val)
	set(__multi_val
		INHERIT   INHERIT_WIN32   INHERIT_APPLE   INHERIT_LINUX   INHERIT_ANDROID
		VARIABLES VARIABLES_WIN32 VARIABLES_APPLE VARIABLES_LINUX VARIABLES_ANDROID VARIABLES_X86_64
		APPEND    APPEND_WIN32    APPEND_APPLE    APPEND_LINUX    APPEND_ANDROID
		${__multi_val_range}
		)

	cmake_parse_arguments(ARGS "${__options}" "${__one_val}" "${__multi_val}" ${ARGN})

	set(var_prefix ${__MAP_PREFIX__}${name})

	# Очищаем кеш
	if (NOT ARGS_NO_CLEAR)
		CreateMap__ClearCache()
	endif()

	# Перетягиваем наследуемые переменные
	foreach(inherit ${ARGS_INHERIT})
		CreateMap__ParseInherit(${inherit})
	endforeach(inherit ${ARGS_INHERIT})

	if(WIN32)
		foreach(inherit ${ARGS_INHERIT_WIN32})
			CreateMap__ParseInherit(${inherit})
		endforeach(inherit ${ARGS_INHERIT_WIN32})
	endif(WIN32)

	if(APPLE)
		foreach(inherit ${ARGS_INHERIT_APPLE})
			CreateMap__ParseInherit(${inherit})
		endforeach(inherit ${ARGS_INHERIT_APPLE})
	endif(APPLE)

	if(LINUX)
		foreach(inherit ${ARGS_INHERIT_LINUX})
			CreateMap__ParseInherit(${inherit})
		endforeach(inherit ${ARGS_INHERIT_LINUX})
	endif(LINUX)

	if(ANDROID)
		foreach(inherit ${ARGS_INHERIT_ANDROID})
			CreateMap__ParseInherit(${inherit})
		endforeach(inherit ${ARGS_INHERIT_ANDROID})
	endif(ANDROID)

	# Разбираем общие переменные
	CreateMap__ParsePairs(false ARGS_VARIABLES)

	# Разбираем WIN32 переменные
	if(WIN32)
		CreateMap__ParsePairs(false ARGS_VARIABLES_WIN32)
	endif(WIN32)

	# Разбираем APPLE переменные
	if(APPLE)
		CreateMap__ParsePairs(false ARGS_VARIABLES_APPLE)
	endif(APPLE)

	# Linux & Android
	if(LINUX)
		CreateMap__ParsePairs(false ARGS_VARIABLES_LINUX)
	endif()
	if(ANDROID)
		CreateMap__ParsePairs(false ARGS_VARIABLES_ANDROID)
	endif()

	# 64-битные
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		CreateMap__ParsePairs(false ARGS_VARIABLES_X86_64)
	endif()

	# Разбераем IFx переменные
	foreach(_var_if RANGE 100)
		set(var ARGS_VARIABLES_IF${_var_if})
		if(DEFINED ${var})
			list(GET ${var} 0 condition)
			list(REMOVE_AT ${var} 0)
			separate_arguments(condition)
			#message("Condition=${condition}")
			if(${condition})
				#message("Success condition=${condition}")
				CreateMap__ParsePairs(false ${var})
			else()
				set(varelse ARGS_VARIABLES_IF${_var_if}ELSE)
				if(DEFINED ${varelse})
					CreateMap__ParsePairs(false ${varelse})
				endif(DEFINED ${varelse})
			endif()
		endif()
	endforeach()

	### APPEND ###
	# Разбираем общие переменные
	CreateMap__ParsePairs(true ARGS_APPEND)

	# Разбираем WIN32 переменные
	if(WIN32)
		CreateMap__ParsePairs(true ARGS_APPEND_WIN32)
	endif(WIN32)

	# Разбираем APPLE переменные
	if(APPLE)
		CreateMap__ParsePairs(true ARGS_APPEND_APPLE)
	endif(APPLE)

	# Разбираем LINUX переменные
	if(LINUX)
		CreateMap__ParsePairs(true ARGS_APPEND_LINUX)
	endif(LINUX)

	# Разбираем ANDROID переменные
	if(ANDROID)
		CreateMap__ParsePairs(true ARGS_APPEND_ANDROID)
	endif(ANDROID)

	# Разбераем IFx переменные
	foreach(_var_if RANGE 100)
		set(var ARGS_APPEND_IF${_var_if})
		if(DEFINED ${var})
			list(GET ${var} 0 condition)
			list(REMOVE_AT ${var} 0)
			separate_arguments(condition)
			#message("Condition=${condition}")
			if(${condition})
				#message("Success condition=${condition}")
				CreateMap__ParsePairs(true ${var})
			else()
				set(varelse ARGS_APPEND_IF${_var_if}ELSE)
				if(DEFINED ${varelse})
					CreateMap__ParsePairs(true ${varelse})
				endif(DEFINED ${varelse})
			endif(${condition})
		endif()
	endforeach()
endfunction(CreateMap name)

# Устанавливает в переменную out список ключей из словаря map
# GetMapKeys(<map> <out>)
function(GetMapKeys map out)
	set(c_prefix ${__MAP_PREFIX__}${map})
	string(LENGTH "${c_prefix}." prefix_length)

	set(_keys)
	foreach(var ${${__MAP_PREFIX__}${map}.__LIST_VARIABLES})
		string(SUBSTRING  ${var} ${prefix_length} -1 wvar)
		append(_keys ${wvar})
	endforeach(var ${${__MAP_PREFIX__}${map}.__LIST_VARIABLES})
	set(${out} ${_keys} PARENT_SCOPE)
endfunction(GetMapKeys map out)

# Копирует в текущеую область видимости переменные из словаря
# GrabMapVariables(
#	<map>			# Имя словаря, из которого надо вытянуть данные
#	[SEPARATE_COMMA]	# Автоматически заменять ',' на ';'
#	[var1 [var2...]]	# Имена переменных, которыое надо вытянуть
#	)
function(GrabMapVariables map)
	set(__options SEPARATE_COMMA)
	set(__one_val)
	set(__multi_val)
	cmake_parse_arguments(ARGS "${__options}" "${__one_val}" "${__multi_val}" ${ARGN})

	foreach(var ${ARGS_UNPARSED_ARGUMENTS})
		if(NOT DEFINED ${__MAP_PREFIX__}${map}.${var})
			message(FATAL_ERROR "Not found variable '${var}' in map '${map}'")
		endif(NOT DEFINED ${__MAP_PREFIX__}${map}.${var})

		set(value ${${__MAP_PREFIX__}${map}.${var}})
		if(ARGS_SEPARATE_COMMA)
			SeparateComma(value)
		endif(ARGS_SEPARATE_COMMA)

		if("${value}" STREQUAL "${__EMPTY__}")
			set(value)
		endif("${value}" STREQUAL "${__EMPTY__}")

		set(${var} ${value} PARENT_SCOPE)
	endforeach(var ${ARGS_UNPARSED_ARGUMENTS})
endfunction(GrabMapVariables map)

# Копирует в текущеую область видимости все переменные из словаря
# GrabAllMapVariables(
#	<map>			# Имя словаря, из которого надо вытянуть данные
#	[SEPARATE_COMMA]	# Автоматически заменять ',' на ';'
#	)
function(GrabAllMapVariables map)
	set(__options SEPARATE_COMMA)
	set(__one_val)
	set(__multi_val)
	cmake_parse_arguments(ARGS "${__options}" "${__one_val}" "${__multi_val}" ${ARGN})

	set(sc)
	if(ARGS_SEPARATE_COMMA)
		set(sc SEPARATE_COMMA)
	endif(ARGS_SEPARATE_COMMA)

	GetMapKeys(${map} keys)
	GrabMapVariables(${map} ${sc} ${keys})
	foreach(key ${keys})
		set(${key} ${${key}} PARENT_SCOPE)
	endforeach(key ${keys})
	set(GrabAllMapVariables_keys ${keys} PARENT_SCOPE)
endfunction(GrabAllMapVariables map)

# Копирует в текущую область видимости один ключ из словаря, автоматически выставляя значение по умолчанию, если ключа нет.
# GrabMapVariableWithDefault(
#	map1	   # Имя словаря, из которого надо вытянуть данные
#	var1       # Имена переменных, которыое надо вытянуть
#   "default"  # Значение, если ключа нет
#	)
function(GrabMapVariableWithDefault map key defaultValue)
	if(DEFINED ${__MAP_PREFIX__}${map}.${key})
		set(value ${${__MAP_PREFIX__}${map}.${key}})
	else()
		set(value ${defaultValue})
	endif()

	if("${value}" STREQUAL "${__EMPTY__}")
		set(value)
	endif()
	set(${key} ${value} PARENT_SCOPE)
endfunction()
