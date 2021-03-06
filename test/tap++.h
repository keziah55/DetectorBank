#ifndef LIB_PERLPP_LIB_TAPPP_TAPPP_H
#define LIB_PERLPP_LIB_TAPPP_TAPPP_H

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <cmath>

namespace TAP {
	namespace details {
		struct skip_all_type {};
		struct no_plan_type {};
		extern std::ostream* output;
		extern std::ostream* error;

        	//Return the variant of "Failed test" or "Failed
	        //(TODO) test" required by whether the current test is
	        //a todo test
		char const * failed_test_msg();
	}
	class fatal_exception : public std::exception {
		std::string message;
		public:
		fatal_exception(const std::string& _message) : message(_message) {
		}
		const char* what() const noexcept{
			return message.c_str();
		}
		~fatal_exception() {
		}
	};
	extern const details::skip_all_type skip_all;
	extern const details::no_plan_type no_plan;
	void plan(unsigned);
	void plan(const details::skip_all_type&, const std::string& = "");
	void plan(const details::no_plan_type&);
	void done_testing();
	void done_testing(unsigned);

	unsigned planned();
	unsigned encountered();

	bool ok(bool, const std::string& = "");
	bool not_ok(bool, const std::string& = "");

	bool pass(const std::string& = "");
	bool fail(const std::string& = "");

	void skip(unsigned, const std::string& = "");
	void bail_out(const std::string& reason);

	int exit_status();
	bool summary();

	void set_output(std::ostream&);
	void set_error(std::ostream&);

	template<typename T> void diag(const T& first) {
		*details::error << "# " << first << std::endl;
	}
	template<typename T1, typename T2> void diag(const T1& first, const T2& second) {
		*details::error << "# " << first << second << std::endl;
	}
	template<typename T1, typename T2, typename T3> void diag(const T1& first, const T2& second, const T3& third) {
		*details::error << "# " << first << second << third << std::endl;
	}
	template<typename T1, typename T2, typename T3, typename T4> void diag(const T1& first, const T2& second, const T3& third, const T4& fourth) {
		*details::error << "# " << first << second << third << fourth << std::endl;
	}
	template<typename T1, typename T2, typename T3, typename T4, typename T5> void diag(const T1& first, const T2& second, const T3& third, const T4& fourth, const T5& fifth) {
		*details::error << "# " << first << second << third << fourth << fifth << std::endl;
	}

	template<typename T> void note(const T& first) {
		*details::output << "# " << first << std::endl;
	}
	template<typename T1, typename T2> void note(const T1& first, const T2& second) {
		*details::output << "# " << first << second << std::endl;
	}
	template<typename T1, typename T2, typename T3> void note(const T1& first, const T2& second, const T3& third) {
		*details::output << "# " << first << second << third << std::endl;
	}
	template<typename T1, typename T2, typename T3, typename T4> void note(const T1& first, const T2& second, const T3& third, const T4& fourth) {
		*details::output << "# " << first << second << third << fourth << std::endl;
	}
	template<typename T1, typename T2, typename T3, typename T4, typename T5> void note(const T1& first, const T2& second, const T3& third, const T4& fourth, const T5& fifth) {
		*details::output << "# " << first << second << third << fourth << fifth << std::endl;
	}

	template<typename T, typename U> typename std::enable_if<!std::is_floating_point<U>::value, bool>::type is(const T& left, const U& right, const std::string& message = "") {
		using namespace TAP::details;
		try {
			bool ret = ok(left == right, message);
			if (!ret) {
				diag(failed_test_msg()," '", message, "'");
				diag("       Got: ", left);
				diag("  Expected: ", right);
			}
			return ret;
		}
		catch(const std::exception& e) {
			fail(message);
			diag(failed_test_msg()," '", message, "'");
			diag("Cought exception '", e.what(), "'");
			diag("       Got: ", left);
			diag("  Expected: ", right);
			return false;
		}
		catch(...) {
			fail(message);
			diag(failed_test_msg()," '", message, "'");
			diag("Caught unknown exception");
			diag("       Got: ", left);
			diag("  Expected: ", right);
			return false;
		}
	}

	template<typename T, typename U> typename std::enable_if<!std::is_floating_point<U>::value, bool>::type isnt(const T& left, const U& right, const std::string& message = "") {
		try {
			return ok(left != right, message);
		}
		catch(const std::exception& e) {
			fail(message);
			diag("In test ", message);
			diag("Caught exception: ", e.what());
			return false;
		}
		catch(...) {
			fail(message);
			diag("In test ", message);
			diag("Cought unknown exception");
			return false;
		}
	}

	template<typename T, typename U> typename std::enable_if<std::is_floating_point<U>::value, bool>::type is(const T& left, const U& right, const std::string& message = "", double epsilon = 0.01) {
		using namespace TAP::details;
		try {
			bool ret = ok(2 * fabs(left - right) / (fabs(left) + fabs(right)) < epsilon);
			if (!ret) {
				diag(failed_test_msg()," '", message, "'");
				diag("       Got: ", left);
				diag("  Expected: ", right);
			}
			return ret;
		}
		catch(const std::exception& e) {
			fail(message);
			diag(failed_test_msg()," '", message, "'");
			diag("Cought exception '", e.what(), "'");
			diag("       Got: ", left);
			diag("  Expected: ", right);
			return false;
		}
		catch(...) {
			fail(message);
			diag(failed_test_msg()," '", message, "'");
			diag("Cought unknown exception");
			diag("       Got: ", left);
			diag("  Expected: ", right);
			return false;
		}
	}

	template<typename T, typename U> typename std::enable_if<std::is_floating_point<U>::value, bool>::type isnt(const T& left, const U& right, const std::string& message = "", double epsilon = 0.01) {
		using namespace TAP::details;
		try {
			bool ret = 2 * fabs(left - right) / (fabs(left) + fabs(right)) > epsilon;
			ok(ret, message);
			return ret;
		}
		catch(const std::exception& e) {
			fail(message);
			diag(failed_test_msg()," '", message, "'");
			diag("Cought exception '", e.what(), "'");
			return false;
		}
		catch(...) {
			fail(message);
			diag(failed_test_msg()," '", message, "'");
			diag("Cought unknown exception");
			return false;
		}
	}

	template<typename T, typename U> bool is_convertible(const std::string& message) {
		return ok(std::is_convertible<T, U>::value, message);
	}

	template<typename T, typename U> bool is_inconvertible(const std::string& message) {
		return ok(!std::is_convertible<T, U>::value, message);
	}

	extern std::string TODO; 

	class todo_guard {
		const std::string value;
		public:
		todo_guard();
		~todo_guard();
	};
}

#ifdef WANT_TEST_EXTRAS

namespace TAP {
	namespace details {
		struct Skip_exception {
			const std::string reason;
			Skip_exception(const std::string& _reason) : reason(_reason) {
			}
		};
		struct Todo_exception {
			const std::string reason;
			Todo_exception(const std::string& _reason) : reason(_reason) {
			}
		};

		void start_block(unsigned);
		unsigned stop_block();

	}

	void skip(const std::string& reason);
	void skip_todo(const std::string& reason);
}

#define TRY(action, name) do {\
		try {\
			action;\
			TAP::pass(name);\
		}\
		catch (const std::exception& e) {\
			TAP::fail(name);\
			note("Caught exception: ", e.what());\
		}\
		catch (...) {\
			TAP::fail(name);\
		}\
	} while (0)

#define FAIL(action, name) do {\
		try {\
			action;\
			TAP::fail(name);\
		}\
		catch (...) {\
			TAP::pass(name);\
		}\
	} while (0)

#define TEST_START(num) {\
		const char* _current_message = NULL;\
		TAP::plan(num);\
		try {

#define TEST_END \
			if (TAP::encountered() < TAP::planned()) {\
				TAP::note("Looks like you planned ", TAP::planned(), " tests but only ran ", TAP::encountered());\
			}\
			else if(TAP::encountered() > TAP::planned()) {\
				TAP::note("Looks like you planned ", TAP::planned(), " tests but ran ", TAP::encountered() - TAP::planned(), " extra");\
			}\
		}\
		catch(TAP::details::Skip_exception& skipper) {\
			TAP::skip(TAP::planned() - TAP::encountered(), skipper.reason);\
		}\
		catch(TAP::details::Todo_exception& todoer) {\
			/*TODO*/\
		}\
		catch(const TAP::fatal_exception& e) {\
			if(_current_message) TAP::fail(_current_message);\
			note("A fatal error occured:,", e.what()," aborting");\
		}\
		catch(const std::exception& e) {\
			if(_current_message) TAP::fail(_current_message);\
			note("Got unknown error: ", e.what());\
		}\
		catch (...) {\
			if(_current_message) TAP::fail(_current_message);\
		}\
		return TAP::exit_status();\
	}

#define BLOCK_START(planned) \
	try {\
		todo_guard foo##planned;\
		TAP::details::start_block(planned);


#define BLOCK_END \
		if (TAP::encountered() != TAP::details::stop_block()) {\
			note("There seems to be a wrong number of tests!");\
		}\
	}\
	catch(TAP::details::Skip_exception& skipper) {\
		TAP::skip(TAP::details::stop_block() - TAP::encountered(), skipper.reason);\
	}\
	catch(TAP::details::Todo_exception& todoer) {\
		note("Can't handle todo properly yet");\
		/*TODO*/\
	}\
	catch(const std::exception& e) {\
		TAP::fail(_current_message);\
		note("Got error: ", e.what());\
	}\
	catch (...) {\
		TAP::fail(_current_message);\
		note("Died with some mysterious error");\
	}

/* This small macro is a main reason for this ugly exercise. I can't introduce a new scope because
 * code subsequent to the declaration should be visible to the rest of the code. At the same time, it
 * must be exception safe. They are quite severe constraints :-(.
 */
#define TRY_DECL(action, new_message) \
	_current_message = new_message;\
	action;\
	TAP::pass(_current_message);\
	_current_message = NULL

#endif /*WANT_TEST_EXTRAS*/


#endif /*LIB_PERLPP_LIB_TAPPP_TAPPP_H*/
