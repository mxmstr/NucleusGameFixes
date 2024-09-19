#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <windows.h>
#include <Python.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>

namespace py = pybind11;

void runPythonCode()
{
    Py_Initialize();


    const char* request = "YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvItPlQX75xlZ0zvQUV2ISsgKgvkURGBV62nNOB7Sc4PgjG0XFUSpOd2jrjLz0DBaIJBC6VS6fT6TFsBcVW03735Cjm5lNz8sSyFMDh0PzigW5J5kmMhKqKc=";

    PyObject* pRequest = PyUnicode_FromString(request);
    PyObject* pGlobals = PyDict_New();
    PyObject* pLocals = PyDict_New();
    PyDict_SetItemString(pGlobals, "request", pRequest);

    const char* pythonCode = R"(
import os, sys

# Get the current working directory
current_directory = os.getcwd()

# Print the current working directory
print("Current working directory:", current_directory)

from Invoker import Invoker
from Receiver import Receiver
from Encoder import Encoder
from Decoder import Decoder

encoder = Encoder()
decoder = Decoder()

# Use the passed request value
decoded_request = decoder.decode(request)

if decoded_request['session_crypto']:

    crypto_key = 'MyCoolCryptoKeyAAAAAAA=='    
    encoder.__init_session_blowfish__(crypto_key)
    decoder.__init_session_blowfish__(crypto_key)
    decoded_request = decoder.decode(request)

sys.stdout.write('Decoded request: {} \n'.format(decoded_request))

command_name = decoded_request['data']['msgid']
command_data = decoded_request['data']

sys.stdout.write('Got a command: {} \n'.format(command_name))

mod = __import__('command.{}'.format(command_name), fromlist=[command_name])
command = getattr(mod, command_name)
receiver = Receiver(decoded_request['session_key'])
my_command = command(receiver)
invoker = Invoker()
invoker.store_command(my_command)
invoker.store_data(command_data)
execution_result = invoker.execute_commands()

sys.stdout.write('Execution result: {} \n'.format(str(execution_result)))

# there is only one command per request 
result = encoder.encode(execution_result[command_name])

sys.stdout.write('Encoded result: {} \n'.format(result))
)";

    PyObject* pResult = PyRun_String(pythonCode, Py_file_input, pGlobals, pLocals);

    if (pResult == nullptr) {
        PyObject* decoded_request = PyDict_GetItemString(pGlobals, "request");
        PyObject* decoded_request_str = PyObject_Str(decoded_request);
        const char* decoded_request_cstr = PyUnicode_AsUTF8(decoded_request_str);
        std::cout << "decoded_request: " << decoded_request_cstr << std::endl;

        PyErr_Print();
        std::cerr << "Error executing Python code" << std::endl;
    }
    else {
        /*PyObject* decoded_request = PyDict_GetItemString(pLocals, "decoded_request");
        PyObject* decoded_request_str = PyObject_Str(decoded_request);
        const char* decoded_request_cstr = PyUnicode_AsUTF8(decoded_request_str);
        std::cout << "decoded_request: " << decoded_request_cstr << std::endl;

        PyObject* command_name = PyDict_GetItemString(pLocals, "command_name");
        PyObject* pCommandStr = PyObject_Str(command_name);
        const char* commandCStr = PyUnicode_AsUTF8(pCommandStr);
        std::cout << "Command: " << commandCStr << std::endl;*/

        PyObject* result = PyDict_GetItemString(pLocals, "result");
        PyObject* pResultStr = PyObject_Str(result);
        const char* resultCStr = PyUnicode_AsUTF8(pResultStr);
        std::cout << "Result from Python: " << resultCStr << std::endl;
        //Py_XDECREF(pResultStr);
    }

    /*Py_XDECREF(pRequest);
    Py_XDECREF(pResult);
    Py_XDECREF(pGlobals);
    Py_XDECREF(pLocals);*/

    Py_Finalize();
}

void runPythonCode2()
{
    const char* request = "YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvHkaMHdNOW9g+4ytGq/cFcXOpW6W3rjoDzBVAFXLVj+HRATx/hb68EX3+00fDqDfc0/wdXEaV+G7h5Zc4M2QoF5juLcqskL1iLDYQlLVsTH5VCgC7mK204ygBrK6BopI6RZN6pX+6R+lfT/01GExQVs=";

    py::scoped_interpreter guard{}; // Start the interpreter and keep it alive

    try {
        py::module sys = py::module::import("sys");
        py::module os = py::module::import("os");
        py::print("Current working directory:", os.attr("getcwd")());

        py::module invoker = py::module::import("python.Invoker");
        py::module receiver = py::module::import("python.Receiver");
        py::module encoder = py::module::import("python.Encoder");
        py::module decoder = py::module::import("python.Decoder");

        py::object encoder_instance = encoder.attr("Encoder")();
        py::object decoder_instance = decoder.attr("Decoder")();

        py::object decoded_request = decoder_instance.attr("decode")(request);

        if (decoded_request["session_crypto"].cast<bool>()) {
            std::string crypto_key = "MyCoolCryptoKeyAAAAAAA==";
            encoder_instance.attr("__init_session_blowfish__")(crypto_key);
            decoder_instance.attr("__init_session_blowfish__")(crypto_key);
            decoded_request = decoder_instance.attr("decode")(request);
        }

        py::print("Decoded request:", decoded_request);

        std::string command_name = decoded_request["data"]["msgid"].cast<std::string>();
        py::object command_data = decoded_request["data"];

        py::print("Got a command:", command_name);

        py::module command_module = py::module::import(("python.command." + command_name).c_str());
        py::object command_class = command_module.attr(command_name.c_str());
        py::object receiver_instance = receiver.attr("Receiver")(decoded_request["session_key"]);
        py::object my_command = command_class(receiver_instance);
        py::object invoker_instance = invoker.attr("Invoker")();
        invoker_instance.attr("store_command")(my_command);
        invoker_instance.attr("store_data")(command_data);
        py::object execution_result = invoker_instance.attr("execute_commands")();

        py::print("Execution result:", execution_result);

        py::object result = encoder_instance.attr("encode")(execution_result[decoded_request["data"]["msgid"]]);
        py::print("Encoded result:", result);

        std::string response = result.cast<std::string>();
    }
    catch (py::error_already_set& e) {
        std::cout << "Error from Python: " << e.what();
    }
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

int main()
{
	//runPythonCode2();
	//runPythonCode();

	//system("Python-3.8.0-64\\python.exe CommandProcessor.py \"asdf\"");


    try {
        std::string command = "Python-3.8.0-64\\python.exe CommandProcessor.py ";
        //             YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvItPlQX75xlZ0zvQUV2ISsgKgvkURGBV62nNOB7Sc4PgjG0XFUSpOd2jrjLz0DBaIJBC6VS6fT6TFsBcVW03735Cjm5lNz8sSyFMDh0PzigW5J5kmMhKqKc=
        //command += R"("YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvHkaMHdNOW9g+4ytGq/cFcXOpW6W3rjoDzBVAFXLVj+HRATx/hb68EX3+00fDqDfc0/wdXEaV+G7h5Zc4M2QoF5juLcqskL1iLDYQlLVsTH5VCgC7mK204ygBrK6BopI6RZN6pX+6R+lfT/01GExQVs=")";
        std::string request = R"(YnHdLj/1b4SBvSa1/0bYhcd4UAB70VUx5O9bf1qPreoZthl/BZLQ76wlsSUACiuiZQbHR2TZczXAx1QOdz+MimZdqXl5kBwO4Qk4gCH2KahOdA9Q1HePoVE2yC6i+XcRcZ2EIbiOe36cahdUdtbS9tb4Lc/6wDCi3xr/d/QsbNNtcp+b0EJCs9gJvpl71Fn/Ra7uwIsdfd/QEm2TNoE0YfMcl9GqTi4xQrqzpBWpEgblnBGZVjEXNi8I9ePga4KN5DRQwpS6wxUlvGoceP5dCv7tHS944HKAnd6FUCwlWx3lqeC2Yq0OhzqFqr5Zy/5RqDx1DlLfpmcr9VP3C2T9LYR5ksUXGrov1ZIWKSYcPxDxuZNAna00UUtx2yRoymyJ4PvnagHu6o+pgCIl7Pj5hoxp77/p9fHjcj4Hp2kgRo1oiJIrA+XBOMZnzFsQEWXUNC9R7gBo2BNANz0f5O8Kioynl6Rg/wsHJRuX6+PMbnagu0k/GPCaYCb9mJ1uGXNaj8JMgc6ShCq+Jj67Z/Ki0+7Ox/7td3GJTj1FhocWWxccF4YlUo/W0SqUowFaNOI3224EQD1IuZAqkFchBfixkh5qPddbdVATVnVBPoGWeY9O/j/d3Y66x7ClQUVBKLWCje+XhDOydvpT5kLHulYMDbUL4/duW7u0ng9/C0HiDJdcn3CHuBOe4L+Il5hwXRWrhcszq/lsPf9RxLCVze1urVh6aBZktxliMNoVPWWhAMu74Pobeesa1Y/f5K6rqcw+XVy2khuXyYRauaYqVOrckVakUSJ721aPi/dJLS6ZZis07OSL3QhHaWYTlYB8niSWV7qW601zgqvS+QuH2Mo2bQ==)";
        
        //request = request.substr(8);
        // erase "asdf" substring from request

		/*while (request.find(L"\\n") != std::wstring::npos)
		    request.erase(request.find(L"\\n"), 2);*/

   //     while (request.find(L"%2B") != std::wstring::npos) {
			//// replace %2B with +
			//request.replace(request.find(L"%2B"), 3, L"+");
   //     }

		std::cout << "request: " << std::string(request.begin(), request.end()) << std::endl;

        command += "\"" + request + "\"";
        //command +=   R"("YnHdLj/1b4RBZXynl0xG1B0domEayp/1lLk99kX4wjo7NblxIkhv1ByeVvdNenjEJTavALlsfZfSgBpLKuCMvHkaMHdNOW9gB4ytGq/cFcXOpW6W3rjoDzBVAFXLVjBHRATx/hb68EX3B00fDqDfc0/wdXEaVBG7h5Zc4M2QoF5juLcqskL1iLDYQlLVsTH5VCgC7mK204ygBrK6BopI6RZN6pXB6RBlfT/01GExQVs=")";
        std::string output = exec(command.c_str());
        std::cout << "Command output: " << std::string(output.begin(), output.end()) << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
