#include "precomp.hpp"
#include "iostream"

using namespace std;
using namespace cv;

void helpParser()
{
    printf("\nThe CommandLineParser class is designed for command line arguments parsing\n"
           "Keys map: \n"
           "Before you start to work with CommandLineParser you have to create a map for keys.\n"
           "    It will look like this\n"
           "    const char* keys =\n"
           "    {\n"
           "        {    s|  string|  123asd |string parameter}\n"
           "        {    d|  digit |  100    |digit parameter }\n"
           "        {    c|noCamera|false    |without camera  }\n"
           "        {    1|        |some text|help            }\n"
           "        {    2|        |333      |another help    }\n"
           "    };\n"
           "Usage syntax: \n"
           "    \"{\" - start of parameter string.\n"
           "    \"}\" - end of parameter string\n"
           "    \"|\" - separator between short name, full name, default value and help\n"
           "Supported syntax: \n"
           "    --key1=arg1  <If a key with '--' must has an argument\n"
           "                  you have to assign it through '=' sign.> \n"
           "<If the key with '--' doesn't have any argument, it means that it is a bool key>\n"
           "    -key2=arg2   <If a key with '-' must has an argument \n"
           "                  you have to assign it through '=' sign.> \n"
           "If the key with '-' doesn't have any argument, it means that it is a bool key\n"
           "    key3                 <This key can't has any parameter> \n"
           "Usage: \n"
           "      Imagine that the input parameters are next:\n"
           "                -s=string_value --digit=250 --noCamera lena.jpg 10000\n"
           "    CommandLineParser parser(argc, argv, keys) - create a parser object\n"
           "    parser.get<string>(\"s\" or \"string\") will return you first parameter value\n"
           "    parser.get<string>(\"s\", false or \"string\", false) will return you first parameter value\n"
           "                                                                without spaces in end and begin\n"
           "    parser.get<int>(\"d\" or \"digit\") will return you second parameter value.\n"
           "                    It also works with 'unsigned int', 'double', and 'float' types>\n"
           "    parser.get<bool>(\"c\" or \"noCamera\") will return you true .\n"
           "                                If you enter this key in commandline>\n"
           "                                It return you false otherwise.\n"
           "    parser.get<string>(\"1\") will return you the first argument without parameter (lena.jpg) \n"
           "    parser.get<int>(\"2\") will return you the second argument without parameter (10000)\n"
           "                          It also works with 'unsigned int', 'double', and 'float' types \n"
           );
}

vector<string> split_string(const string& str, const string& delimiters)
{
    vector<string> res;
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    string::size_type pos     = str.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos)
    {

        res.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delimiters, pos);
        if (str[pos + 1] == '|' && str[pos] == '|')
        {
            res.push_back("");
            if(str[pos + 2] == '|')
                res.push_back("");
        }
        if (str[pos + 1] == '\0')
            res.push_back("");
        pos = str.find_first_of(delimiters, lastPos);
    }

    return res;
}

CommandLineParser::CommandLineParser(int argc, const char* argv[], const char* keys)
{

    std::string keys_buffer;
    std::string values_buffer;
    std::string buffer;
    std::string curName;
    std::vector<string> keysVector;
    std::vector<string> paramVector;
    std::map<std::string, std::vector<std::string> >::iterator it;
    size_t flagPosition;
    int currentIndex = 1;
    bool isFound = false;
    bool withNoKey = false;
    bool hasValueThroughEq = false;

    keys_buffer = keys;
    while (!keys_buffer.empty())
    {

        flagPosition = keys_buffer.find_first_of('}');
        flagPosition++;
        buffer = keys_buffer.substr(0, flagPosition);
        keys_buffer.erase(0, flagPosition);

        flagPosition = buffer.find('{');
        if (flagPosition != buffer.npos)
            buffer.erase(flagPosition, (flagPosition + 1));

        flagPosition = buffer.find('}');
        if (flagPosition != buffer.npos)
            buffer.erase(flagPosition);

        paramVector = split_string(buffer, "|");
        buffer = paramVector[0];
        if (atoi(buffer.c_str()) == 0)
            buffer = buffer + '|' + paramVector[1];

        paramVector.erase(paramVector.begin(), paramVector.begin() + 2);
        data[buffer] = paramVector;
    }

    buffer.clear();
    keys_buffer.clear();
    paramVector.clear();
    for (int i = 1; i < argc; i++)
    {
        if (!argv[i])
            break;
        curName = argv[i];
        if (curName.find('-') == 0 && ((curName[1] < '0') || (curName[1] > '9')))
        {
            while (curName.find('-') == 0)
                curName.erase(curName.begin(), (curName.begin() + 1));
        }
            else
                withNoKey = true;
        if (curName.find('=') != curName.npos)
        {
            hasValueThroughEq = true;
            buffer = curName;
            curName.erase(curName.find('='));
            buffer.erase(0, (buffer.find('=') + 1));
        }

        for(it = data.begin(); it != data.end(); it++)
        {
            keys_buffer = it->first;
            keysVector = split_string(keys_buffer, "| ");
            if (keysVector.size() == 1)
                keysVector.push_back("");
            values_buffer = it->second[0];
            if (((curName == keysVector[0]) || (curName == keysVector[1])) && hasValueThroughEq)
            {
                it->second[0] = buffer;
                isFound = true;
                break;
            }

            if (!hasValueThroughEq && (values_buffer.find("false") == values_buffer.npos) &&
                ((curName == keysVector[0]) || (curName == keysVector[1])))

            {
                it->second[0] = argv[++i];
                isFound = true;
                break;
            }

            if (!hasValueThroughEq &&  (values_buffer.find("false") != values_buffer.npos)
                && ((curName == keysVector[0]) || (curName == keysVector[1])))

            {
                it->second[0] = "true";
                isFound = true;
                break;
            }

            if (withNoKey)
            {
                std::string noKeyStr = it->first;
                if(atoi(noKeyStr.c_str()) == currentIndex)
                {
                    it->second[0] = curName;
                    currentIndex++;
                    isFound = true;
                    break;
                }
            }
        }

        withNoKey = false;
        hasValueThroughEq = false;
        if(!isFound)
            printf("The current parameter is not defined: %s\n", curName.c_str());
        isFound = false;
    }


}

bool CommandLineParser::has(const std::string& keys)
{
    std::map<std::string, std::vector<std::string> >::iterator it;
    std::vector<string> keysVector;
    for(it = data.begin(); it != data.end(); it++)
    {
        keysVector = split_string(it->first, "| ");
        if (keysVector.size() == 1)
            keysVector.push_back("");
        if ((keys == keysVector[0]) || (keys == keysVector[1]))
            return true;
    }
    return false;
}

std::string CommandLineParser::getString(const std::string& keys)
{
    std::map<std::string, std::vector<std::string> >::iterator it;
    std::vector<string> valueVector;

    for(it = data.begin(); it != data.end(); it++)
    {
        valueVector = split_string(it->first, "| ");
        if (valueVector.size() == 1)
            valueVector.push_back("");
        if ((keys == valueVector[0]) || (keys == valueVector[1]))
            return it->second[0];
    }
    return string();
}

template<typename _Tp>
 _Tp CommandLineParser::fromStringNumber(const std::string& str)//the default conversion function for numbers
{
    const char* c_str=str.c_str();
    if ((!isdigit(c_str[0]))
        &&
        (
            (c_str[0]!='-') || (strlen(c_str) <= 1) || ( !isdigit(c_str[1]) )
        )
    )

    {
        printf("This string cannot be converted to a number. Zero will be returned %s\n ", str.c_str());
        return _Tp();
    }

    return  getData<_Tp>(str);
}

 void CommandLineParser::printParams()
 {
     std::map<std::string, std::vector<std::string> >::iterator it;
     std::vector<string> keysVector;
     for(it = data.begin(); it != data.end(); it++)
     {
         keysVector = split_string(it->first, "| ");
         if (keysVector.size() == 1)
             keysVector.push_back("");
         printf("\t%s [%8s] (%12s - by default) - %s\n", keysVector[0].c_str(),
                keysVector[1].c_str(), it->second[0].c_str(), it->second[1].c_str());
     }
 }

template<>
bool CommandLineParser::get<bool>(const std::string& name, bool space_delete)
{
    std::string str_buf = getString(name);
    if (space_delete)
    {
        while (str_buf.find_first_of(' ') == 0)
            str_buf.erase(0, 1);
        while (str_buf.find_last_of(' ') == (str_buf.length() - 1))
            str_buf.erase(str_buf.end() - 1, str_buf.end());
    }
    if (str_buf == "false")
        return false;
    return true;
}
template<>
std::string CommandLineParser::analizeValue<std::string>(const std::string& str, bool space_delete)
{
    if (space_delete)
    {
        std::string str_buf = str;
        while (str_buf.find_first_of(' ') == 0)
            str_buf.erase(0, 1);
        while (str_buf.find_last_of('-') == (str.length() - 1))
            str_buf.erase(str_buf.end() - 1, str_buf.end());
        return str_buf;
    }
    return str;
}

template<>
int CommandLineParser::analizeValue<int>(const std::string& str, bool space_delete)
{
    return fromStringNumber<int>(str);
}

template<>
unsigned int CommandLineParser::analizeValue<unsigned int>(const std::string& str, bool space_delete)
{
    return fromStringNumber<unsigned int>(str);
}

template<>
float CommandLineParser::analizeValue<float>(const std::string& str, bool space_delete)
{
    return fromStringNumber<float>(str);
}

template<>
double CommandLineParser::analizeValue<double>(const std::string& str, bool space_delete)
{
    return fromStringNumber<double>(str);
}
