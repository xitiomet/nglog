#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include "vt100.h"

using namespace std;

struct Param
{
    string key;
    string value;
};

struct Request
{
    string url;
    string timestamp;
    vector<Param> params;
};

struct PreparedLog
{
    vector<Request> requests;
    vector<string> columns;
    float total_lines;
};

void unique_add(vector<string>& v, string& s)
{
    unsigned int i;
    bool found = false;
    for (i=0; i < v.size(); i++)
    {
        if (v.at(i) == s)
        {
            found = true;
        }
    }
    if (!found)
    {
        v.push_back(s);
    }
}

string get_param(vector<Param> v, string key)
{
    unsigned int i;
    for (i=0; i < v.size(); i++)
    {
        Param e = v.at(i);
        if (key == e.key)
        {
            return e.value;
        }
    }
    return "";
}

PreparedLog prepareLog(istream& input_stream)
{
    string line;
    unsigned int linenum = 0;
    PreparedLog return_log;
    while (getline (input_stream, line))
    {
        linenum++;
	// unique struct for line
        Request this_request;
        istringstream linestream(line);
        string item;
        int itemnum = 0;
        while (getline (linestream, item, ' '))
        {
            itemnum++;
	    if (itemnum == 4)
	    {
	       this_request.timestamp = item.substr(1);
	    } 
            if (itemnum == 7)
            {
                int qloc = item.find_first_of("?");
                string url_params;
                string url;

                if (qloc != string::npos)
                {
                    this_request.url = item.substr(0, qloc);
                    url_params = item.substr(qloc+1);

                    int paramnumber = 0;
                    istringstream paramstream(url_params);
                    string param;

                    // Check parameters to see what kind of colums we will have to create
                    while (getline (paramstream, param, '&'))
                    {
                        int eqloc = param.find_first_of("=");
                        if (eqloc != string::npos)
                        {
                            Param x;

                            x.key = param.substr(0,eqloc);
                            x.value = param.substr(eqloc+1);

                            this_request.params.push_back(x);

                            // if not in column db add it
                            unique_add(return_log.columns, x.key);
                        }
                    }

                } else {
                    // No Parameters lets just get the url
                    this_request.url = item;
                }
                return_log.requests.push_back(this_request);
            }
        }
    }
    return_log.total_lines = linenum;
    return return_log;
}

void write_csv(PreparedLog& plog, ostream& outfile)
{
    outfile << "\"timestamp\",\"url\",";
    unsigned int c;
    for (c=0; c < plog.columns.size(); c++)
    {
        string f = plog.columns.at(c);
        outfile << "\"" << f << "\"";
        if (c+1 < plog.columns.size())
        {
            outfile << ",";
        }
    }
    outfile << endl;

    unsigned int i;
    for (i=0; i < plog.requests.size(); i++)
    {
        Request n = plog.requests.at(i);
        outfile << "\"" << n.timestamp << "\"," << "\"" << n.url << "\",";
        unsigned int b;
        for (b=0; b < plog.columns.size(); b++)
        {
            string e = plog.columns.at(b);
            outfile << "\"" << get_param(n.params, e) << "\"";
            if (b+1 < plog.columns.size())
            {
                outfile << ",";
            }
        }
        outfile << endl;
    }
}

string view_vector(vector<string>& v)
{
    unsigned int i;
    ostringstream oss;
    for (i=0; i < v.size(); i++)
    {
        oss << v.at(i);
        if (i+1 < v.size())
        {
            oss << ", ";
        }
    }
    return oss.str();
}


int main (int argc, char *argv[])
{
    int i;
    bool std_in = false;
    bool std_out = false;
    bool show_help = true;
    bool input_set = false;
    bool output_set = false;
    bool quiet = false;
    
    string outFilename;
    string inFilename;
    
    for (i=0; i<argc; i++)
    {
        string thisParam = argv[i];
        if (!thisParam.compare("--help"))
        {
            show_help = true;
        }
        
        if (!thisParam.compare("--pipe"))
        {
            std_in = true;
            inFilename = "";
            std_out = true;
            inFilename = "";
            
            input_set = true;
            output_set = true;
        }
        
        if (!thisParam.compare("--quiet"))
        {
            quiet = true;
        }
        
        if (!thisParam.compare("--stdin"))
        {
            std_in = true;
            inFilename = "";
            
            input_set = true;
        }
        
        if (!thisParam.compare("--stdout"))
        {
            std_out = true;
            outFilename = "";
            
            output_set = true;
        }

        if (!thisParam.compare("-o"))
        {
            outFilename = argv[i+1];
            std_out = false;
            
            output_set = true;
        }

        if (!thisParam.compare("-i"))
        {
            inFilename = argv[i+1];
            std_in = false;
            
            input_set = true;
        }
    }
    
    if (input_set && output_set)
    {
        show_help = false;
    }
    
    if (show_help)
    {
        cerr << "Usage: nglog [OPTION] -i [FILE] -o [FILE]" << endl;
        cerr << "Process nginx logs for urls into csv files. Used to debug restfull api calls through nginx" << endl;
        cerr << endl;
        cerr << "Options:" << endl;
        cerr << "  --pipe           Take input from stdin, and send output to stdout" << endl;
        cerr << "  --stdin          Take input from stdin" << endl;
        cerr << "  --stdout         Send output to stdout" << endl;
        cerr << "  --quiet          No Messages" << endl;
        cerr << "  -i               Specify Input file (NGINX LOG)" << endl;
        cerr << "  -o               Specify Output file (CSV)" << endl << endl;
        return 0;
    }
    
    if (!quiet)
    {
        cerr << set_bold(true) << set_colors(VT_RED, VT_DEFAULT) << "NGINX Weblog Parser" << set_bold(false) << endl;
        cerr << set_colors(VT_YELLOW, VT_DEFAULT)<< "Processing " << inFilename << "....";
    }

    istream *input_stream;
    ifstream inFile;
    if (!std_in)
    {    
        inFile.open(inFilename.c_str());
        input_stream = &inFile;
    } else {
        input_stream = &cin;
    }
    PreparedLog plog = prepareLog(*input_stream);

    if (!quiet)
    {
        cerr << set_colors(VT_DEFAULT, VT_DEFAULT) << "OK!" << set_colors(VT_DEFAULT, VT_DEFAULT) << endl;
        cerr << set_colors(VT_YELLOW, VT_DEFAULT) << "Lines Processed: " << set_colors(VT_DEFAULT, VT_DEFAULT) << plog.total_lines << endl;
        cerr << set_colors(VT_YELLOW, VT_DEFAULT) << "Columns Detected: " << set_colors(VT_DEFAULT, VT_DEFAULT) << view_vector(plog.columns) << endl;
        cerr << finalize;
    }
    
    if (!std_out)
    {
        ofstream outfile (outFilename.c_str());
        write_csv(plog, outfile);
        outfile.close();
    } else {
        write_csv(plog, cout);
    }
    return 0;
}

