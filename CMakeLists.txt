cmake_minimum_required (VERSION 3.22)
project(SearchServer LANGUAGES CXX)
add_executable (SearchServer 		"search-server/main.cpp"
						"search-server/document.cpp" "search-server/document.h"
						"search-server/read_input_functions.cpp" "search-server/read_input_functions.h"
						"search-server/request_queue.cpp" "search-server/request_queue.h"
						"search-server/paginator.h" "search-server/log_duration.h"
						"search-server/test_example_functions.cpp" "search-server/test_example_functions.h"
						"search-server/search_server.cpp" "search-server/search_server.h"
						"search-server/string_processing.cpp" "search-server/string_processing.h"
						"search-server/remove_duplicates.cpp" "search-server/remove_duplicates.h" 
						"search-server/process_queries.cpp" "search-server/process_queries.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SearchServer PROPERTY CXX_STANDARD 17)
endif()