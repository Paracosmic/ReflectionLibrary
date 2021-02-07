#pragma once
#include <map>
#include <vector>
#include <tuple>
#include <string>
#include "FunctionHandle.h"
#include "VirtualVariable.h"

#define ReflectMember(V,CType,cl,F) 		V.Reflect_Member_Function(GenName(F),cl,&CType::F,{})

#define ReflectMember_Name(V,CType,cl,name,F) 		V.Reflect_Member_Function(name,cl,&CType::F,{})

#define ReflectGlobalStatic(V,F) 		V.Reflect_Static_Function(GenName(F),F,{})


#define ReflectStatic(V,CType,F) 		V.Reflect_Static_Function(GenName(F),&CType::F,{})

class VirtualFunctionUtility
{
public:

	std::vector<std::shared_ptr<FunctionHandle>> function_container;
	std::map<std::string, std::shared_ptr<FunctionHandle>> function_map;

	//class for primitive virtual variables
	Primitives& primitives;


	VirtualFunctionUtility(Primitives& maptest) : primitives(maptest) {};

	bool find(std::string& name);


	//Not safe!
	FunctionHandle* GetFunctionHandle(std::string name);

	void CacheFunction(std::string name);

	template< typename F, typename... Args>
	size_t Reflect_Static_Function(std::string name, F f, const std::vector<std::string>& parameter_info = {})
	{

		return Reflect_Static_Function_impl(function_container, name, f, parameter_info);

	};

	template< typename CC, typename F, typename... Args>
	size_t Reflect_Member_Function(std::string name, CC& c, F f, const std::vector<std::string>& parameter_info = {})
	{
		return Reflect_Member_Function_impl(function_container, name, c, f, parameter_info);
	};


	template< typename R, typename... Args>
	size_t Reflect_Static_Function_impl(std::vector<std::shared_ptr<FunctionHandle>>& function_container, std::string name, R(*ptr)(Args...), const std::vector<std::string>& parameter_info)
	{
		//TODO PROPERLY HANDLE DELETION
		auto f = new MetaStaticFunctionData< R, Args...>;
		f->function_pointer = ptr;
		function_container.emplace_back(std::move(f));

		size_t index = function_container.size() - 1;

		function_container[index]->function_name = name;

		function_container[index]->parameter_count = std::tuple_size<std::tuple<Args...>>::value;

		function_container[index]->Parameter_Info = parameter_info;


		function_map[name] = function_container[index];

		return index;
	};


	template<typename R, typename CC, typename... Args>
	size_t Reflect_Member_Function_impl(std::vector<std::shared_ptr<FunctionHandle>>& function_container, std::string name, CC& c, R(CC::* ptr)(Args...), const std::vector<std::string>& parameter_info)
	{
		//TODO PROPERLY HANDLE DELETION
		auto f = new MetaFunctionData< R, CC, Args...>;
		f->function_pointer = ptr;

		function_container.emplace_back(std::move(f));

		size_t index = function_container.size() - 1;

		function_container[index]->function_name = name;


		function_container[index]->parameter_count = std::tuple_size<std::tuple<Args...>>::value;

		function_container[index]->Parameter_Info = parameter_info;

		function_map[name] = function_container[index];


		BindClass(&c, sizeof(c), name);


		return index;
	};



	void PrintFunctions();

	//binds the class object to the virtualized member function
	void BindClass(void * ptr, size_t size, std::string function_name);


	template <typename... T>
	const void operator()(const std::string& function_name, const T&... args)
	{

		function_map[function_name]->ParseInput(args...);

	};


	template <typename... T>
	const void Call(const std::string& function_name, const T&... args)
	{

		function_map[function_name]->ParseInput(args...);

	};
	std::string GetTokenString(std::string line) 
	{

	};

	//TODO move or find a more generic function
	std::vector<std::string> Tokenize(const std::string& line)
	{
		bool is_string = false;
		std::vector<std::string> tokens;
		std::string token = "";
		for (const auto& chr : line)
		{
			if (chr == '"') {
				is_string = !is_string;
				//continue;
			}

			if (!is_string)
			{
				if (chr != ' ')
				{
					//	token.append(std::string(&chr));
					token.push_back(chr);
					//	token.append(&chr);
				}
				else {

					tokens.push_back(token);
					token = "";

				}
			}
			else
			{

				token.push_back(chr);

			}

		}

		tokens.push_back(token);

		return tokens;

	};


	void FindAndReplaceVirtualVariables(std::vector<std::string>& tokens)
	{
		//check each token
		for (auto& token : tokens)
		{
			//is a virtual variable
			if (token[0] == '$')
			{
				std::string var_name = token.substr(1,token.size());



				token = primitives.TryGet(var_name);

			//	std::cout << "";


			}
		}
	
	}

	static void FindAndReplaceVirtualVariables(Primitives& primitives, std::vector<std::string>& tokens)
	{
		//check each token
		for (auto& token : tokens)
		{
			//is a virtual variable
			if (token[0] == '$')
			{
				std::string var_name = token.substr(1, token.size());



				token = primitives.TryGet(var_name);

				//	std::cout << "";


			}
		}

	}
	/*
	ret : true if executed, false if invalid
	param0 : name of function
	param1 : outbound error message
	*/
	const bool TryExecute(const std::string& command, std::string& out_log)
	{

		//First break the string up into tokens 
		std::vector<std::string> tokens = Tokenize(command);

		//token[0] is standard for the function name
		//store this in a string for later
		std::string name = tokens[0];

		//erase token[0] as it is no longer needed as a token
		tokens.erase(tokens.begin());

		//look for the function
		auto found = function_map.find(name);


		//function found, set parameters
		if (found != function_map.end())
		{

			//Call the function 
			function_map[name]->CallFunction(tokens);
			//return true since it was properly executed 
			return true;
		}
		else if (found == function_map.end())
		{

			std::string errout = name; errout.append(" is an invalid function.\n");
			out_log.append(errout);

		}
		return false;
	};

	/*
	ret : true if executed, false if invalid
	 param0 : name of function
	Uses std::cout to log errors
	*/
	const bool TryExecute(const std::string& command)
	{

		//First break the string up into tokens 
		std::vector<std::string> tokens = Tokenize(command);

		FindAndReplaceVirtualVariables(tokens);
		
		//token[0] is standard for the function name
		//store this in a string for later
		std::string name = tokens[0];
		//erase token[0] as it is no longer needed as a token

		tokens.erase(tokens.begin());
	
		if (command[0] == '>') {

			
			std::string com = "";
			
			for (auto& itr : tokens)
			{
				com.append(itr).append(" ");
			}
			tokens.clear();
			tokens = { com };

			//	tokens.erase(tokens.begin());
		}
		
	

		//look for the function
		auto found = function_map.find(name);


		//function found, set parameters
		if (found != function_map.end())
		{

			//Call the function 
			function_map[name]->CallFunction(tokens);
			//return true since it was properly executed 
			return true;
		}
		else if (found == function_map.end())
		{

			std::string errout = name; errout.append(" is an invalid function.\n");
			std::cout << errout;
		}
		return false;
	};

};
