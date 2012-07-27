/** \file
 *******************************************************************
 * $Author: santai.ww $
 *
 * $LastChangedBy: santai.ww $
 *
 * $Revision: 516 $
 *
 * $LastChangedDate: 2011-04-08 23:43:26 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: CMWebServerMain.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

/**
 * CMWebServerMain.cpp
 * It listens to `hostname`:8000 to accept incoming http request.
 */
#include <anet/anet.h>
#include <signal.h> 
#include <queue>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string>
#include <sys/mman.h>
#include <fcntl.h>
#include "CMTree.h"
#include "CMClient.h"
#include "CMTCPClient.h"

using namespace anet;
using namespace std;
using namespace clustermap;

class CCMWebServer
{
    public:
        CCMWebServer() : m_pcmtree(NULL), m_cpRoot(NULL) { }
        ~CCMWebServer() {if(m_pcmtree) delete m_pcmtree; m_pcmtree = NULL;}

        int32_t parseURL(string &s);
        int32_t setHTML_Index(stringstream &ss);
        int32_t setHTML_Head(stringstream &ss);
        int32_t setHTML_Head_Bottom(stringstream &ss);
        int32_t setHTML_Top(stringstream &ss);
        int32_t setHTML_Bottom(stringstream &ss);   
        CMTree *m_pcmtree; //节点结构
        time_t m_subtime;

        int32_t showTreeInfo(stringstream &ss);
        int32_t travelTree(CMRelation* root, int indent, stringstream &ss);
        int32_t showQueryInfo(stringstream &ss);
        int32_t showClusterInfo(stringstream &ss);
        int32_t showNodeInfo(CMISNode* cpNode, stringstream &ss);
        int32_t showSubClusterInfo(stringstream &ss);
        int32_t showBadInfo(stringstream &ss);    
        int32_t travelTree_BadNode(CMRelation* root, int indent, stringstream &ss, int &BadNodeCount);
        int32_t showNode_WatchPoint(CMClient *pClient, stringstream &ss);

        string m_urifile;
        string m_uriquery;
        string m_clustername;
        string m_nodespec;
        bool m_isNodeQuery;
        bool m_isAutoRefresh;
        int32_t m_RefreshTime;
        bool m_ShoworHide;
        bool m_ShoworHideBad;
        bool m_hasAbnormal;
        bool m_hasUnvalid;
        bool m_hasTimeout;
        bool m_isAllBad;  
        string m_badClustername;

        CMRelation *m_cpRoot;

};

bool globalStopFlag = false; 
struct HTTPRequestEntry {
    Connection * _connection;
    HTTPPacket * _packet;
    HTTPRequestEntry() : _connection(NULL), _packet(NULL) {}
};

struct RequestQueue {
    ThreadCond _condition;
    queue<HTTPRequestEntry> _queue;
    size_t _queueLimit;
    size_t _waitCount;
    RequestQueue() : _queueLimit(200), _waitCount(0) {}
} globalQueue;

const char *root = NULL;
char *data_logo = NULL;
size_t size_logo = 0;
class HTTPServerAdapter : public IServerAdapter 
{
    public:
        HTTPServerAdapter() { }
        virtual ~HTTPServerAdapter() { }
    public:
        IPacketHandler::HPRetCode handlePacket(Connection *connection, Packet *packet) 
        {
            if (packet->isRegularPacket()) { //handle request
                HTTPPacket *request = dynamic_cast<HTTPPacket*>(packet);
                if (NULL == request) { 
                } else {
                    CMHandlePacket(connection, request);
                }
            } else { //control command received
                packet->free(); //free packet if finished
            }
            return IPacketHandler::FREE_CHANNEL;
        }

        void CMHandlePacket(Connection *connection, HTTPPacket* request) 
        {
            MutexGuard guard(&(globalQueue._condition));
            if (globalQueue._queue.size() >= globalQueue._queueLimit) {
                HTTPPacket *reply = new HTTPPacket;
                assert(reply);
                reply->setPacketType(HTTPPacket::PT_RESPONSE);
                reply->setStatusCode(503);
                reply->setReasonPhrase("Service Unavailable");
                if (!connection->postPacket(reply)) {
                    reply->free();
                }
                request->free();
            } else {//add connection, request to globalQueue for later processing
                HTTPRequestEntry entry;
                connection->addRef();//add a reference count for later using
                entry._connection = connection;
                entry._packet = request;
                globalQueue._queue.push(entry);
                if (globalQueue._waitCount > 0) {
                    globalQueue._condition.signal();
                }
            }
        }
};

class RequestProcessor : public Runnable 
{
    public:
        void run(Thread* thread, void *args) 
        {
            pClient = (CMClient*)args;
            while (!globalStopFlag) {
                taskIteration();
            }
        }

        void taskIteration() 
        {
            globalQueue._condition.lock();
            while(!globalStopFlag && globalQueue._queue.size() == 0) {
                globalQueue._waitCount ++;
                globalQueue._condition.wait();
                globalQueue._waitCount --;
            }
            if (globalStopFlag) {
                globalQueue._condition.unlock();
                return;
            }
            HTTPRequestEntry entry = globalQueue._queue.front();
            globalQueue._queue.pop();
            globalQueue._condition.unlock();
            doReply(entry);
        }

        int32_t doReply(HTTPRequestEntry &entry) 
        {

            HTTPPacket *reply = new HTTPPacket();
            reply->setPacketType(HTTPPacket::PT_RESPONSE);
            const char *uri = entry._packet->getURI();
            assert(uri);
            if (uri[0] != '/') {
                reply->setStatusCode(404);
            } else {
                stringstream ss;
                ss << uri;
                if (uri[strlen(uri) - 1] == '/') {
                    ss << "index.html";
                }
                //printf("\nuri: %s\n", ss.str().c_str());
                string s = ss.str();
                void* ptree = (void*)cWebServer.m_pcmtree;
                if(pClient->getTree(ptree, cWebServer.m_subtime) != 0) {
                    printf("get tree failed\n");
                    return -1;
                }
                if (cWebServer.parseURL(s) != 0)
                    return -1;
                ss.clear();
                ss.str("");
                if (cWebServer.m_urifile == "index.html") {
                    cWebServer.setHTML_Index(ss);
                } else if (cWebServer.m_urifile == "top.html") {
                    cWebServer.setHTML_Top(ss);
                } else if (cWebServer.m_urifile == "bottom.html") {
                    cWebServer.setHTML_Bottom(ss);
                } else if (cWebServer.m_urifile == "watchpoint.html") {
                    cWebServer.showNode_WatchPoint(pClient, ss);
                }
                if (cWebServer.m_urifile == "isearch_cm_top.jpg") {
                    if (MAP_FAILED != data_logo) {
                        reply->setStatusCode(200);
                        reply->setBody(data_logo, size_logo);
                    } else { 
                        int fd = open("isearch_cm_top.jpg", O_RDONLY);
                        if ( -1 != fd ) {
                            struct stat s;
                            if (-1 != fstat(fd, &s) && S_ISREG(s.st_mode)) {
                                size_t size = s.st_size;
                                char *data = (char*)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
                                if (MAP_FAILED != data) {
                                    reply->setStatusCode(200);
                                    reply->setBody(data, size);
                                    munmap(data, size);
                                }
                            }
                        }
                    }
                } else {
                    reply->setStatusCode(200);            
                    reply->setBody(ss.str().c_str(), ss.str().size());
                    reply->addHeader("Content-Type", "text/html");
                }
            }
            reply->setKeepAlive(entry._packet->isKeepAlive());
            if (!reply->isKeepAlive()) {
                entry._connection->setWriteFinishClose(true);
            }
            reply->addHeader("Server", "ClusterMap_httpd/1.0 ");
            if (!entry._connection->postPacket(reply)) {
                reply->free();
            };
            entry._packet->free();
            entry._connection->subRef();
            return 0;
        }

        CCMWebServer cWebServer; 
        CMClient *pClient;
};

void singalHandler(int seg)
{
    globalStopFlag = true;
}

void doProcess(unsigned int port, unsigned int num, CMClient& client) 
{
    Transport transport;
    transport.start(); //using multithreads mode of anet    

    Thread *threads = new Thread[num];
    RequestProcessor *runnables = new RequestProcessor [num];
    assert(threads);
    assert(runnables);
    for (long i=0; i<num; i++) {
        threads[i].start(runnables + i, (void*)&client);
    }

    HTTPPacketFactory factory;
    HTTPStreamer streamer(&factory);
    HTTPServerAdapter serverAdapter;
    stringstream ss;
    char hostname[1024];
    if (gethostname(hostname, 1024) != 0) {
        transport.stop();
        transport.wait();
        delete [] threads;
        delete [] runnables;
        exit(-1);
    }
    //    ss << "tcp:" << hostname << ":" << port;
    ss << "tcp:" << ":" << port;
    string str = ss.str();
    const char *spec = str.c_str();
    IOComponent *ioc = transport.listen(spec, &streamer, &serverAdapter);
    if (ioc == NULL) {
        printf("create listen port error\n");
        transport.stop();
        transport.wait();
        delete [] threads;
        delete [] runnables;
        return;
    }
    printf("webserver start ok\n");

    while (!globalStopFlag) {
        usleep(100000);
    }
    transport.stop();
    transport.wait();

    globalQueue._condition.lock();
    while (globalQueue._queue.size()) {
        HTTPRequestEntry entry = globalQueue._queue.front();
        entry._packet->free();
        entry._connection->subRef();
        globalQueue._queue.pop();
    }
    globalQueue._condition.broadcast();
    globalQueue._condition.unlock();

    for (long i=0; i<num; i++) {
        threads[i].join();
    }

    delete [] threads;
    delete [] runnables;
}

int32_t CCMWebServer::parseURL(string &s)
{
    // default values
    m_urifile = "";
    m_uriquery = "";
    m_clustername = "";
    m_nodespec = "";
    m_isAutoRefresh = true;
    m_RefreshTime = 10000;
    m_ShoworHide = true;
    m_ShoworHide = true;
    m_hasAbnormal = true;
    m_hasUnvalid = true;
    m_hasTimeout = true;
    m_isAllBad = true;
    m_badClustername = "";    

    int i = 0, j = 0, k = 0;
    string key, value;
    i = s.find('?', 0); // i point to '?' or '&'
    if (i == (int)string::npos) {
        m_urifile = s.substr(1, s.size()-1);
        m_uriquery = "";
        return 0;
    } else {
        m_urifile = s.substr(1, i-1);
        m_uriquery = s.substr(i, s.size()-i);
    }
    while (i != (int)string::npos) {
        //cout << "paramter count: " << count++ << "; "; 
        j = s.find('=', i+1); // j point to '='
        k = s.find('&', i+1); // k point to next '&'
        if (k == (int)string::npos) { // last (key, value) pair
            //cout << "Last Pair: ";  
            k = s.size();
        }
        //cout << "paramter = " << s.substr(i, k - i) << " ";
        if (j == (int)string::npos || j > k || j == i + 1) { 
            // to avoid no key ("&tcp&" or "&tcp&ar=on" or "&&ar=on" or "&=tcp&" or "&tcp" or "&=tcp")
            //cout << "noKey\n";
            if (k == (int)s.size()) {
                i = (int)string::npos;
                break;
            }
            i = k;
            continue;
        }        
        if (j == k - 1) { // to avoid no value ("&n=&" or "&n=") situation
            //cout << "noValue\n";
            if (k == (int)s.size()) {
                i = (int)string::npos;
                break;
            }
            i = k;
            continue;
        }
        key.assign(s, i + 1, j - i - 1);        
        value.assign(s, j + 1, k - j - 1);
        i = k;
        if (i == (int)s.size()) {
            i = (int)string::npos;
        }
        if (key == "c") { // string m_clustername;
            m_clustername = value;
            //cout << "clustername = " << m_clustername << "\n";
            continue;
        }
        if (key == "n") { // string m_nodespec;
            m_nodespec = value;
            //cout << "nodespec = " << m_nodespec << "\n";
            continue;
        }
        if (key == "r") { // bool m_isAutoRefresh;
            if (value == "false") {
                m_isAutoRefresh = false;
            } else {
                m_isAutoRefresh = true;
            }
            //cout << "isAutoRefresh = " << m_isAutoRefresh << "\n";
            continue;          
        }    
        if (key == "rt") { // int32_t m_RefreshTime;
            int32_t t = -1;
            t = atoi(value.c_str());
            if (t > 0) {
                m_RefreshTime = t;
            }
            //cout << "RefreshTime = " << m_RefreshTime << "\n";
            continue;
        }    
        if (key == "sh") { // bool m_ShoworHide;
            if (value == "false") {
                m_ShoworHide = false;
            } else {
                m_ShoworHide = true;
            }
            //cout << "ShoworHide = " << m_ShoworHide << "\n";
            continue;
        }
        if (key == "sb") { // bool m_ShoworHideBad;
            if (value == "false") {
                m_ShoworHideBad = false;
            } else {
                m_ShoworHideBad = true;
            }
            //cout << "ShoworHideBad = " << m_ShoworHideBad << "\n";
            continue;
        }
        if (key == "ab") { // bool m_hasAbnormal;
            if (value == "false") {
                m_hasAbnormal = false;
            } else {
                m_hasAbnormal = true;
            }
            //cout << "hasAbnormal = " << m_hasAbnormal << "\n";
            continue;
        }    
        if (key == "un") { // bool m_hasUnvalid;
            if (value == "false") {
                m_hasUnvalid = false;
            } else {
                m_hasUnvalid = true;
            }
            //cout << "hasUnvalid = " << m_hasUnvalid << "\n";
            continue;
        }  
        if (key == "to") { // bool m_hasTimeout;
            if (value == "false") {
                m_hasTimeout = false;
            } else {
                m_hasTimeout = true;
            }
            //cout << "hasTimeout = " << m_hasTimeout << "\n";
            continue;
        }
        if (key == "ba") { // bool m_isAllBad;
            if (value == "false") {
                m_isAllBad = false;
            } else {
                m_isAllBad = true;
            }
            //cout << "isAllBad = " << m_isAllBad << "\n";
            continue;
        }    
        if (key == "bc") { // string m_badClustername;
            m_badClustername = value;
            //cout << "badClustername = " << m_badClustername << "\n";
            continue;
        }
    }	 
    return 0;
}

int32_t CCMWebServer::setHTML_Index(stringstream &ss)
{
    ss << "<html>\n";
    ss << "  <frameset rows=\"80, *\">\n";
    ss << "    <frame name=\"topframe\" noresize=\"noresize\" scrolling=\"auto\" frameborder=\"0\" src=\"/top.html" << m_uriquery << "\">\n";
    ss << "    <frame name=\"bottomframe\" noresize=\"noresize\" scrolling=\"auto\" frameborder=\"0\" src=\"/bottom.html" << m_uriquery << "\">\n";
    ss << "  </frameset>\n";
    ss << "</html>\n";

    return 0;
}

int32_t CCMWebServer::setHTML_Head(stringstream &ss)
{
    ss << "<head>\n";
    ss << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\" />\n";
    ss << "<title>ClusterMap Monitor</title>\n";
    ss << "<script type=\"text/javascript\">\n";
    ss << "function Submit() {\n";
    ss << "    var strhref = \"\";\n";
    ss << "    var text = htmlEncode(document.getElementById(\"input_clustername\").value);\n";
    ss << "    strhref += \"?c=\" + text;\n";
    ss << "    text = htmlEncode(document.getElementById(\"input_node\").value);\n";  
    ss << "    strhref += \"&n=\" + text;\n";
    ss << "    if (document.getElementById(\"check_refresh\").checked == true) {\n";
    ss << "        strhref += \"&r=true\";\n";
    ss << "        strhref += \"&rt=\" + document.getElementById(\"refresh_time\").value;\n";
    ss << "    } else {\n";
    ss << "        strhref += \"&r=false\";\n";
    ss << "        strhref += \"&rt=\";\n";
    ss << "    }\n";
    ss << "    if (window.top.bottomframe.document.getElementById(\"check_showhide\").value == \"false\") {\n";
    ss << "        strhref += \"&sh=false\";\n";
    ss << "    } else {\n";
    ss << "        strhref += \"&sh=true\";\n";
    ss << "    }\n";
    ss << "    if (window.top.bottomframe.document.getElementById(\"check_showhide_bad\").value == \"false\") {\n";
    ss << "        strhref += \"&sb=false\";\n";
    ss << "    } else {\n";
    ss << "        strhref += \"&sb=true\";\n";
    ss << "    }\n";
    ss << "    if (document.getElementById(\"check_abnormal\").checked == true) {\n";
    ss << "        strhref += \"&ab=true\";\n";
    ss << "    } else {\n";
    ss << "        strhref += \"&ab=false\";\n";
    ss << "    }\n";
    ss << "    if (document.getElementById(\"check_unvalid\").checked == true) {\n";
    ss << "        strhref += \"&un=true\";\n";
    ss << "    } else {\n";
    ss << "        strhref += \"&un=false\";\n";
    ss << "    }\n";
    ss << "    if (document.getElementById(\"check_timeout\").checked == true) {\n";
    ss << "        strhref += \"&to=true\";\n";
    ss << "    } else {\n";
    ss << "        strhref += \"&to=false\";\n";
    ss << "    }\n";
    ss << "    if (document.getElementById(\"check_allbad\").checked == true) {\n";
    ss << "        strhref += \"&ba=true\";\n";
    ss << "        document.getElementById(\"input_badclustername\").value=\"\";\n";
    ss << "    } else {\n";
    ss << "        strhref += \"&ba=false\";\n";
    ss << "    }\n";
    ss << "    text = htmlEncode(document.getElementById(\"input_badclustername\").value);\n";
    ss << "    strhref += \"&bc=\" + text;\n";
    ss << "    window.top.location.href = \"index.html\" + strhref;\n";
    ss << "}\n";
    ss << "function clickSubmitCluster(clustername) {\n";
    ss << "    document.getElementById(\"input_clustername\").value = document.getElementById(\"text_clustername\").value;\n";
    ss << "    if (clustername.length != 0) {\n";
    ss << "        document.getElementById(\"input_clustername\").value = clustername;\n";
    ss << "    }\n";
    ss << "    if (document.getElementById(\"input_clustername\").value.length == 0) {\n";
    ss << "        alert(\"Query String is Null!\");\n";
    ss << "        return;\n";
    ss << "    }\n";
    ss << "    document.getElementById(\"input_node\").value = \"\";\n";
    ss << "    Submit();\n";
    ss << "}\n";
    ss << "function clickSubmitNode() {\n";
    ss << "    document.getElementById(\"input_node\").value = document.getElementById(\"text_clustername\").value;\n";
    ss << "    if (document.getElementById(\"input_node\").value.length == 0) {\n";
    ss << "        alert(\"Query String is Null!\");\n";
    ss << "        return;\n";
    ss << "    }\n";
    ss << "    document.getElementById(\"input_clustername\").value = \"\";\n";
    ss << "    Submit();\n";
    ss << "}\n";
    ss << "function clickBadCluster() {\n";
    ss << "    if (document.getElementById(\"input_badclustername\").value.length == 0) {\n";
    ss << "        alert(\"Query String is Null!\");\n";
    ss << "        return;\n";
    ss << "    }\n";
    ss << "    Submit();\n";
    ss << "}\n";
    ss << "function htmlEncode (str) {\n";
    ss << "    if (str.length == 0)\n";
    ss << "        return \"\";\n";  
    ss << "    if (str.length > 64) {\n";
    ss << "        alert(\"Query String is too Long!\");\n";
    ss << "        return \"\";\n";
    ss << "    }\n";
    ss << "    if (str.indexOf(\"\\&\") != -1\n"; 
    ss << "     || str.indexOf(\"<\") != -1\n";
    ss << "     || str.indexOf(\">\") != -1\n";
    ss << "     || str.indexOf(\" \") != -1\n";
    ss << "     || str.indexOf(\"\\\\\") != -1\n";
    ss << "     || str.indexOf(\"\\\'\") != -1\n";
    ss << "     || str.indexOf(\"\\\"\") != -1) {\n";
    ss << "        alert(\"Query String contains illegal character! Including \\& < > \\\\ \\\' \\\" space\");\n";
    ss << "        return \"\";\n";
    ss << "    }\n";
    ss << "    return str;\n";   
    ss << "}\n";
    ss << "</script>\n";
    ss << "</head>\n";

    return 0;
}

int32_t CCMWebServer::setHTML_Head_Bottom(stringstream &ss)
{
    ss << "<head>\n";
    ss << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\" />\n";
    ss << "<title>ClusterMap Monitor</title>\n";
    ss << "<script type=\"text/javascript\">\n";
    ss << "function display(id) {\n";
    ss << "    var traget=document.getElementById(id);\n";
    ss << "    if(traget.style.display == \"none\"){\n";
    ss << "        traget.style.display = \"\";\n";
    ss << "        if (id==\"cluster_info\") document.getElementById(\"check_showhide\").value=\"true\";\n";
    ss << "        if (id==\"bad_info\") document.getElementById(\"check_showhide_bad\").value=\"true\";\n";
    ss << "    }else{\n";
    ss << "        traget.style.display = \"none\";\n";
    ss << "        if (id==\"cluster_info\") document.getElementById(\"check_showhide\").value=\"false\";\n";
    ss << "        if (id==\"bad_info\") document.getElementById(\"check_showhide_bad\").value=\"false\";\n";
    ss << "    }\n";
    ss << "    window.top.topframe.Submit();\n";
    ss << "}\n";
    ss << "</script>\n";
    ss << "</head>\n";

    return 0;    
}

int32_t CCMWebServer::setHTML_Top(stringstream &ss)
{
    ss << "<html>\n";
    setHTML_Head(ss);
    ss << "<body>\n";
    ss << "<table>\n";
    ss << "  <tr>\n";
    ss << "    <td colspan=\"2\" align=\"center\" valign=\"middle\">\n";
    // index_top
    ss << "        <table>\n";
    ss << "          <tr>\n";
    ss << "            <td>\n";
    ss << "            <img src=\"isearch_cm_top.jpg\" alt=\"ISEARCH\">\n";
    ss << "            </td>\n";
    ss << "            <td align=\"center\">\n";
    ss << "        	 <font color=\"blue\"><b>ClusterMap<br>Monitor</b></font>\n";
    ss << "            </td>\n";
    ss << "          </tr>\n";
    ss << "        </table>\n";
    // index_top
    ss << "    </td>\n";
    ss << "    <td>\n";
    // index_query
    showQueryInfo(ss);
    // index_query
    ss << "    </td>\n";
    ss << "  </tr>\n";
    ss << "</table>\n";
    ss << "</body>\n";
    ss << "</html>\n";

    return 0;
}

int32_t CCMWebServer::setHTML_Bottom(stringstream &ss)
{
    ss << "<html>\n";
    setHTML_Head_Bottom(ss);
    ss << "<body";
    if (m_isAutoRefresh && m_RefreshTime > 0) {
        ss << " onload=\"setTimeout(\'window.location.reload()\', " << m_RefreshTime << ");\"";
    }
    ss << ">\n";
    ss << "<table>\n";
    ss << "  <tr>\n";
    ss << "  <td rowspan=\"3\" align=\"left\" valign=\"top\">\n";
    // index_tree
    showTreeInfo(ss);
    // index_tree
    ss << "  </td>\n";
    ss << "  <td align=\"left\" valign=\"top\" >\n";
    ss << "    <table>\n";
    ss << "    <tr><td>\n";
    // index_cluster
    showClusterInfo(ss);
    // index_cluster
    ss << "    </td></tr>\n";
    ss << "    </table>\n";
    ss << "    <hr>\n";
    ss << "    <table>\n";
    ss << "    <tr><td>\n";
    // index_subcluster
    showSubClusterInfo(ss);
    // index_subcluster
    ss << "    </td></tr>\n";
    ss << "    </table>\n";
    ss << "    <hr>\n";
    ss << "    <table>\n";
    ss << "    <tr><td>\n";
    // index_bad
    showBadInfo(ss);
    // index_bad
    ss << "    </td></tr>\n";
    ss << "    </table>\n";
    ss << "  </td>\n";
    ss << "  </tr>\n";
    ss << "</table>\n";
    ss << "</body>\n";
    ss << "</html>\n";

    return 0;
}

int32_t CCMWebServer::showTreeInfo(stringstream &ss) 
{
    ss << "        <table>\n";
    ss << "          <tr>\n";
    ss << "            <td align=\"center\">Recent State Update Time:</td>\n";
    ss << "          </tr>\n";
    ss << "          <tr>\n";
    ss << "            <td align=\"center\">\n";
    struct tm* pt = localtime(&m_subtime);
    ss << pt->tm_year+1900 << "-"<< pt->tm_mon+1 << "-" << pt->tm_mday << " " << pt->tm_hour << ":" << pt->tm_min << ":" << pt->tm_sec << "\n";
    ss << "            </td>\n";
    ss << "          </tr>\n";
    ss << "          <tr>\n";
    ss << "            <td align=\"center\"><br>ClusterMap Overview</td>\n";
    ss << "          </tr>\n";
    ss << "          <tr><td align=\"center\"><font size=\"2\" color=\"gray\">( # Alive, # Dead )</font></td></tr>\n";
    ss << "          <tr>\n";
    ss << "            <td>\n";
    ss << "              <table>\n";
    m_pcmtree->getRootRelation(m_cpRoot);
    travelTree(m_cpRoot, 0, ss);
    ss << "              </table>\n";
    ss << "            </td>\n";
    ss << "          </tr>\n";
    ss << "        </table>\n";
    return 0;   
}

int32_t CCMWebServer::travelTree(CMRelation* root, int indent, stringstream &ss)
{
    if (indent > 0) {
        ss << "                <tr><td>";
        ss << "&nbsp;";
        for(int i = 1; i <= indent-2; i++) {
            ss << "|&nbsp;&nbsp;&nbsp;";
        }
        if (indent >= 2)
            ss << "|---";
        ss << "<a href=\"\" onclick=\"window.top.topframe.clickSubmitCluster(\'" << root->cpCluster->m_name  << "\'); return false;\"><b>" << root->cpCluster->m_name << "</b></a>";
        int alive_count = 0;
        CMISNode* cpNode = root->cpCluster->m_cmisnode;
        for(int32_t j = 0; j < root->cpCluster->node_num; j++, cpNode++) {
            if (cpNode->m_state.state == normal) {
                alive_count++;
            }
        }    
        ss << "<font size=\"2\" color=\"";
        if (alive_count != root->cpCluster->node_num) { ss << "red"; } else { ss << "gray"; };
        ss << "\">&nbsp;( " << alive_count << ", " << root->cpCluster->node_num-alive_count << " )</font>";     
        ss << "&nbsp;&nbsp;</td></tr>\n";
    }

    if(root->child)
        travelTree(root->child, indent+1, ss);

    if(root->sibling)
        travelTree(root->sibling, indent, ss);

    return 0;
}

int32_t CCMWebServer::showQueryInfo(stringstream &ss)
{
    ss << "        <table>\n";
    ss << "          <tr>\n";
    ss << "            <td><input type=\"text\" id=\"text_clustername\"></td>\n";
    ss << "            <td><input type=\"button\" onclick=\"clickSubmitCluster(\'\');\" value=\"Search Cluster\"></td>\n";
    ss << "            <td><input type=\"button\" onclick=\"clickSubmitNode();\" value=\"Search Node\"/></td>\n";
    ss << "            <td><input type=\"hidden\" id=\"input_clustername\"";
    if (m_clustername != "") { ss << " value=\"" << m_clustername << "\""; }
    ss << "></td>\n";
    ss << "            <td><input type=\"hidden\" id=\"input_node\"";
    if (m_nodespec != "") { ss << " value=\"" << m_nodespec << "\""; }
    ss << "></td>\n";
    ss << "            <td><font color=\"gray\" size=\"2\">Input:ClusterName/Node(protocol:ip:port)</font></td>\n";
    ss << "            <td><input type=\"checkbox\" id=\"check_refresh\"";
    if (m_isAutoRefresh && m_RefreshTime > 0) { ss << " checked = \"checked\""; }
    ss << " onclick=\"Submit();\"/>AutoRefresh</td>\n";
    ss << "            <td id=\"RefreshTime\"";
    if (!m_isAutoRefresh || m_RefreshTime <= 0) { ss << " style=\"display:none;\""; }
    ss << ">&nbsp;RefreshTime:\n";
    ss << "            <select name=\"refresh_time\" id=\"refresh_time\" onchange=\"Submit();\">\n";
    int32_t time[10] = {1000, 3000, 5000, 10000, 20000, 30000, 60000, 90000, 120000, 180000}; 
    for (int i = 0; i < 10; i++) {
        ss << "              <option value=\"" << time[i] << "\"";
        if (m_RefreshTime == time[i])  { ss << " selected=\"selected\""; }
        ss << ">" << time[i]/1000 << "s</option>\n";
    }
    ss << "            </select>\n";
    ss << "            </td>\n";
    ss << "          </tr>\n";
    ss << "        </table>\n";
    ss << "        <table>\n";
    ss << "          <tr>\n";
    ss << "            <td>Unavailable Node:</td>\n";
    ss << "            <td><input type=\"radio\" name=\"radio_allbad\" id=\"check_allbad\" value=\"true\" onclick=\"Submit();\"";
    if (m_isAllBad) { ss << " checked=\"checked\""; }
    ss << ">All Clusters</td>\n";    
    ss << "            <td colspan=\"3\">&nbsp;&nbsp;<input type=\"radio\" name=\"radio_allbad\"";
    if (!m_isAllBad) { ss << " checked=\"checked\""; };
    ss << ">Search:<input type=\"text\" id=\"input_badclustername\"";
    if (m_badClustername != "") { ss << " value=\"" << m_badClustername << "\""; }
    ss << "><input type=\"button\" onclick=\"clickBadCluster();\" value=\"Search Cluster\"/></td>\n";
    ss << "            <td>&nbsp;&nbsp;<input type=\"checkbox\" id=\"check_abnormal\" onclick=\"Submit();\"";
    if (m_hasAbnormal) { ss << " checked=\"checked\""; }
    ss << ">Abnormal</td>\n";
    ss << "            <td>&nbsp;&nbsp;<input type=\"checkbox\" id=\"check_unvalid\" onclick=\"Submit();\"";
    if (m_hasUnvalid) { ss << " checked=\"checked\""; }
    ss << ">Unvalid</td>\n";
    ss << "            <td>&nbsp;&nbsp;<input type=\"checkbox\" id=\"check_timeout\" onclick=\"Submit();\"";
    if (m_hasTimeout) { ss << " checked=\"checked\""; }
    ss << ">Timeout</td>\n";
    ss << "            <td>&nbsp;&nbsp;<a href=\"watchpoint.html\" target=\"_top\">Nodes' WatchPoint</a></td>\n";
    ss << "          </tr>\n";
    ss << "        </table>\n";

    return 0;
}

int32_t CCMWebServer::showClusterInfo(stringstream &ss)
{
    ss << "        <table><tr><td><font color=\"blue\">Cluster Information</font>&nbsp;";   
    bool bSucc = true;
    CMISNode *pNode = NULL;
    CMCluster *pCMCluster = NULL;
    if (m_nodespec != "") {
        size_t i = m_nodespec.find(':', 0);
        string proto = "";
        eprotocol eproto = tcp;
        string ip = "";
        uint16_t port = 0;
        if (i == string::npos) {
            bSucc = false;
            printf("format error, need( protocol:ip:port ), input = %s\n", m_nodespec.c_str());
        } else {
            proto = m_nodespec.substr(0, i);
            if (proto == "tcp") {
                eproto = tcp;
            } else if (proto == "udp") {
                eproto = udp;
            } else if (proto == "http") {
                eproto = http;
            } else {
                bSucc = false;
                printf("protocol error, support tcp,udp,http, input = %s\n", proto.c_str());
            }
            size_t j = m_nodespec.find(':', i+1);
            if (j == string::npos) {
                bSucc = false;
                printf("format error, need( protocol:ip:port ), input = %s\n", m_nodespec.c_str());
            } else {
                ip = m_nodespec.substr(i+1, j-i-1);
                port = (uint16_t)atol(m_nodespec.substr(j+1).c_str());
                if(port == 0) {
                    bSucc = false;
                    printf("port error, input = %u\n", port);
                }
            }
        }
        if (bSucc) {
            if (m_pcmtree->getNode((char*)ip.c_str(), port, eproto, pNode) != 0 || pNode == NULL)
                bSucc = false;                
        }     
        if (bSucc) {
            ss << " ( Show below Query[ <font color=\"blue\">node = " << m_nodespec << "</font> ] search result. )";
            m_clustername = pNode->m_clustername;
            m_pcmtree->getCluster(m_clustername.c_str(), pCMCluster);
        } else {
            ss << " ( Show below Default Cluster, since Query[ <font color=\"blue\">node = " << m_nodespec << "</font> ] has NO search result. )";
            // set default Cluster
            pCMCluster = m_cpRoot->child->cpCluster;
            m_clustername = pCMCluster->m_name;
        }
    } else {    
        if (m_clustername == "") ss << " ( Show below Default Cluster. )";
        if (m_pcmtree->getCluster(m_clustername.c_str(), pCMCluster) != 0 || pCMCluster == NULL) {
            if (m_clustername != "") 
                ss <<" ( Show below Default Cluster, since Query[ <font color=\"blue\">cluster = " << m_clustername << "</font> ] has NO search result. )";
            // set default Cluster
            pCMCluster = m_cpRoot->child->cpCluster;
            m_clustername = pCMCluster->m_name;
        } else {
            ss << " ( Show below Query[ <font color=\"blue\">cluster = " << m_clustername << "</font> ] search result. )";      
        }
    }
    ss << "<br>\n";
    // m_clustername's parent
    vector<string> vec_clustername;
    vec_clustername.clear();
    int32_t relationid;
    CMRelation *pRelation = NULL;
    if (pCMCluster->m_cmisnode) {
        relationid = pCMCluster->m_cmisnode->m_relationid;
        m_pcmtree->getRelation(relationid, pRelation);
        while (pRelation) {
            //cout << pRelation->cpCluster->m_name;
            vec_clustername.push_back(pRelation->cpCluster->m_name);           
            pRelation = pRelation->parent;
        }          
        ss << "        ";
        for (int j = vec_clustername.size() - 2; j >= 0; j--) {
            ss << "<a href=\"\" onclick=\"window.top.topframe.clickSubmitCluster(\'" << vec_clustername[j] << "\'); return false;\"><b>" << vec_clustername[j] << "</b></a>";
            if (j != 0) ss <<" &gt; ";
        }
        ss << "\n";
    }
    // ClusterInfo's title
    ss << "        <br>Cluster <a href=\"\" onclick=\"window.top.topframe.clickSubmitCluster(\'" << m_clustername << "\'); return false;\"><b>";
    ss << m_clustername;
    ss << "</b></a> has " << ((pCMCluster->node_num >= 0)?pCMCluster->node_num:0) << " Nodes.&nbsp;&nbsp;&nbsp;&nbsp;\n";
    ss << "        <input type=\"button\" onclick=\"display(\'cluster_info\');\" value=\"show/hide Cluster detail\">\n";
    ss << "        <input type=\"hidden\" id=\"check_showhide\" value=\"" << (m_ShoworHide ? "true" : "false") << "\">\n";
    ss << "        </td></tr></table>\n";
    // cluster_info
    ss << "        <table width=\"100%\" id = \"cluster_info\"";
    if (!m_ShoworHide) { ss << " style=\"display:none;\""; }
    ss << ">\n";
    ss << "          <tr><td><table frame=\"box\" rules=\"all\">\n";
    ss << "          <tr>\n";
    ss << "            <td align=\"center\">Node</td>\n";
    ss << "            <td align=\"center\">State</td>\n";
    ss << "            <td align=\"center\">Start</td>\n";
    ss << "            <td align=\"center\">Last Dead Begin</td>\n";
    ss << "            <td align=\"center\">Last Dead End</td>\n";
    ss << "            <td align=\"center\">Total Dead Time</td>\n";
    ss << "            <td align=\"center\">Total Alive Time</td>\n";
    ss << "            <td align=\"center\">Cluster</td>\n";
    ss << "          </tr>\n";
    if (m_nodespec != "" && bSucc) {
        ss << "          <tr>";
        showNodeInfo(pNode, ss);
        ss << "</tr>\n";
    } else {
        CMISNode* cpNode = pCMCluster->m_cmisnode;
        for(int32_t j = 0; j < pCMCluster->node_num; j++, cpNode++) {
            ss << "          <tr>";
            showNodeInfo(cpNode, ss);
            ss << "</tr>\n";  
        }
    }   
    ss << "          </table></td>\n";
    ss << "          </tr>\n";
    ss << "        </table>\n";
    return 0;
}

int32_t CCMWebServer::showNodeInfo(CMISNode* cpNode, stringstream &ss)
{
    ss << "<td align=\"center\" bgcolor=\"";
    if (cpNode->m_state.state == normal) {
        ss << "lawngreen\">";
    } else {
        ss << "red\">";
    }
    if (cpNode->m_protocol == tcp) {
        ss << "tcp:";
    } else if (cpNode->m_protocol == udp) {
        ss << "udp:";
    } else if (cpNode->m_protocol == http) {
        ss << "http:";
    } else {
        ss << "unknown:";
    }
    ss << cpNode->m_ip;
    ss << ":" << cpNode->m_port;
    ss << "</td>";

    ss << "<td align=\"center\">";
    if (cpNode->m_state.state == normal) {
        ss << "Normal";
    } else if (cpNode->m_state.state == abnormal) {
        ss << "Abnormal";
    } else if (cpNode->m_state.state == unvalid) {
        ss << "Unvalid";
    } else if (cpNode->m_state.state == timeout) {
        ss << "Timeout";
    } else if (cpNode->m_state.state == uninit) {
        ss << "Uninit";
    } else {
        ss << "Unknown";
    }
    ss << "</td>";

    if (cpNode->m_startTime != 0) {
        ss << "<td align=\"center\">";
        struct tm* pt = localtime(&cpNode->m_startTime);
        ss << pt->tm_year+1900 << "-"<< pt->tm_mon+1 << "-" << pt->tm_mday << " " << pt->tm_hour << ":" << pt->tm_min << ":" << pt->tm_sec;
        ss << "</td>";
        ss << "<td align=\"center\">";
        if (cpNode->m_dead_begin != 0) {
            pt = localtime(&cpNode->m_dead_begin);
            ss << pt->tm_year+1900 << "-"<< pt->tm_mon+1 << "-" << pt->tm_mday << " " << pt->tm_hour << ":" << pt->tm_min << ":" << pt->tm_sec;
        } else {
            ss << "/";
        }
        ss << "</td>";
        ss << "<td align=\"center\">";
        if (cpNode->m_dead_end != 0 && cpNode->m_state.state == normal) {
            pt = localtime(&cpNode->m_dead_end);
            ss << pt->tm_year+1900 << "-"<< pt->tm_mon+1 << "-" << pt->tm_mday << " " << pt->tm_hour << ":" << pt->tm_min << ":" << pt->tm_sec; 
        } else {
            ss << "/";
        }
        ss << "</td>";
        time_t now = time(NULL);
        ss << "<td align=\"center\">";
        time_t dead_time = cpNode->m_dead_time;
        if (cpNode->m_state.state != normal && now > cpNode->m_dead_begin) {                
            dead_time += now - cpNode->m_dead_begin;
        }
        if (dead_time > 0)
            ss << dead_time/3600 << ":" << dead_time % 3600 / 60 << ":" << dead_time % 3600 % 60;
        else 
            ss << "/";
        ss << "</td>";
        ss << "<td align=\"center\">";
        time_t alive_time = now - cpNode->m_startTime - dead_time;
        if (alive_time > 0)
            ss << alive_time/3600 << ":" << alive_time % 3600 / 60 << ":" << alive_time % 3600 % 60;
        else 
            ss << "/";
        ss << "</td>";
    } else {
        ss << "<td align=\"center\">Never Started!</td><td align=\"center\">/</td><td align=\"center\">/</td><td align=\"center\">/</td><td align=\"center\">/</td>";
    }
    ss << "<td align=\"center\">";
    if (cpNode->m_clustername != NULL && cpNode->m_clustername[0] != '\0') {
        ss << cpNode->m_clustername;
    } else {
        ss << "/";
    }
    ss << "</td>";

    return 0;  
}

int32_t CCMWebServer::showSubClusterInfo(stringstream &ss) 
{
    ss << "        <table>\n";
    CMCluster *pCMCluster = NULL;
    CMCluster **ppCMCluster = NULL;
    int32_t relationid = 0;
    int32_t SubClusterSize = 0;
    if (m_pcmtree->getCluster(m_clustername.c_str(), pCMCluster) != 0 || pCMCluster == NULL) {
        return -1;
    } else {
        if (pCMCluster->m_cmisnode) {            
            relationid = pCMCluster->m_cmisnode->m_relationid;
            if (m_pcmtree->getSubCluster(relationid, ppCMCluster, SubClusterSize)) {
                return -1;
            }
            ss << "          <tr><td><font color=\"blue\">SubCluster:</font>&nbsp;&nbsp;<a href=\"\" onclick=\"window.top.topframe.clickSubmitCluster(\'" << pCMCluster->m_name << "\'); return false;\"><b>";
            ss << pCMCluster->m_name << "</b></a>&nbsp;has&nbsp;";
            ss << SubClusterSize << "&nbsp;SubCluster.</td></tr>\n";
            if (ppCMCluster != NULL && SubClusterSize > 0) {
                for (int i = 0; i < SubClusterSize; i++) {
                    ss << "          <tr><td>&nbsp;&nbsp;&nbsp;&nbsp;";
                    ss << "<a href=\"\" onclick=\"window.top.topframe.clickSubmitCluster(\'" << ppCMCluster[i]->m_name << "\'); return false;\"><b>";
                    ss << ppCMCluster[i]->m_name;
                    ss << "</b></td></tr>\n";                
                }
            }
            m_pcmtree->freeSubCluster(ppCMCluster);
        }
    }
    ss << "        </table>\n";
    return 0;
}

int32_t CCMWebServer::showBadInfo(stringstream &ss)
{
    // temp stringstream
    stringstream strs;
    strs << "        <table id=\"bad_info\" frame=\"box\" rules=\"all\"";
    if (!m_ShoworHideBad) { strs << " style=\"display:none;\""; }
    strs << ">\n";
    strs << "          <tr>\n";
    strs << "            <td align=\"center\">Node</td>\n";
    strs << "            <td align=\"center\">State</td>\n";
    strs << "            <td align=\"center\">Start</td>\n";
    strs << "            <td align=\"center\">Last Dead Begin</td>\n";
    strs << "            <td align=\"center\">Last Dead End</td>\n";
    strs << "            <td align=\"center\">Total Dead Time</td>\n";
    strs << "            <td align=\"center\">Total Alive Time</td>\n";
    strs << "            <td align=\"center\">Cluster</td>\n";
    strs << "          </tr>\n";
    bool hasSearchResult = false;
    int BadNodeCount = 0;
    if (m_hasAbnormal || m_hasUnvalid || m_hasTimeout) {
        CMCluster *pCMCluster = NULL;
        if (!m_isAllBad && m_badClustername != "" && m_pcmtree->getCluster(m_badClustername.c_str(), pCMCluster) == 0 &&
                pCMCluster != NULL) {
            hasSearchResult = true;    
            CMISNode* cpNode = pCMCluster->m_cmisnode;
            for(int32_t j = 0; j < pCMCluster->node_num; j++, cpNode++) {
                if (m_hasAbnormal && cpNode->m_state.state == abnormal || m_hasUnvalid && cpNode->m_state.state == unvalid || m_hasTimeout && cpNode->m_state.state == timeout) {
                    BadNodeCount++;
                    strs << "          <tr>";
                    showNodeInfo(cpNode, strs);
                    strs << "</tr>\n";
                }
            }
        } else {
            travelTree_BadNode(m_cpRoot, 0, strs, BadNodeCount);
        }
    }
    strs << "        </table>\n";
    //
    ss << "        <table>\n";
    ss << "          <tr><td><font color=\"blue\">Unavailable Node</font>&nbsp;";
    if (!m_isAllBad) {
        if (hasSearchResult) ss << " ( Show below Query[ <font color=\"blue\">cluster = " << m_badClustername << "</font> ] search result. )\n";
        else ss << " ( Show below All Clusters, since Query[ <font color=\"blue\">cluster = " << m_badClustername << "</font> ] has NO search result. )\n";
    } else {
        ss << " ( Show below All Clusters. )\n";
    }
    ss << "          </td></tr>\n";
    ss << "        </table>\n";
    ss << "        <table><tr><td>There are " << BadNodeCount << " Unavailable Nodes.&nbsp;&nbsp;&nbsp;&nbsp;\n";
    ss << "          <input type=\"button\" onclick=\"display(\'bad_info\');\" value=\"show/hide Unavailable Node\">\n";
    ss << "          <input type=\"hidden\" id=\"check_showhide_bad\" value=\"" << (m_ShoworHideBad ? "true" : "false") << "\">\n";
    ss << "        </td></tr></table>\n";
    ss << strs.str();
    return 0;
}

int32_t CCMWebServer::travelTree_BadNode(CMRelation* root, int indent, stringstream &ss, int &BadNodeCount)
{
    if (indent > 0) {
        CMCluster *pCMCluster = root->cpCluster;
        CMISNode* cpNode = pCMCluster->m_cmisnode;
        for(int32_t j = 0; j < pCMCluster->node_num; j++, cpNode++) {
            if (m_hasAbnormal && cpNode->m_state.state == abnormal || m_hasUnvalid && cpNode->m_state.state == unvalid || m_hasTimeout && cpNode->m_state.state == timeout) {
                BadNodeCount++;
                ss << "          <tr>";
                showNodeInfo(cpNode, ss);
                ss << "</tr>\n";
            }
        }                
    }
    if(root->child)
        travelTree_BadNode(root->child, indent+1, ss, BadNodeCount);

    if(root->sibling)
        travelTree_BadNode(root->sibling, indent, ss, BadNodeCount);

    return 0;
}

int32_t CCMWebServer::showNode_WatchPoint(CMClient *pClient, stringstream &ss)
{
    CMTCPClient cClientM, cClientS;
    vector<string> server_spec;
    pClient->getCMTCPServer(server_spec);
    if (server_spec.size() >= 1) {
        cClientM.setServerAddr(server_spec[0].c_str());
        cClientM.start();
    }
    if (server_spec.size() >= 2) {
        cClientS.setServerAddr(server_spec[1].c_str());
        cClientS.start();
    }
    CNodetype cNodeType;
    CProtocol cProtocol;
    getNodeInput in;
    getNodeOutput out;
    in.type = cmd_get_info_watchpoint;
    if(in.nodeToBuf() <= 0) {
        ss << "Communication Error!";
        return -1; 
    }               
    char* replyBody = NULL;
    int32_t replyLen = 0;
    if(cClientM.send(in.buf, in.len) == 0 && cClientM.recv(replyBody, replyLen) == 0
            || cClientS.send(in.buf, in.len) == 0 && cClientS.recv(replyBody, replyLen) == 0) {
        ss << "<html>\n";
        ss << "  <head></head>\n";
        ss << "  <body>\n";
        char* p = replyBody;
        //eMessageType type = (eMessageType)(*p);
        p += 1;
        int32_t node_num = ntohl(*(int32_t*)p);
        p += 4;
        ss << "    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"\" onclick=\"history.back(); return false;\">Back</a>\n";
        ss << "    &nbsp;&nbsp;&nbsp;&nbsp;<a href=\"\" onclick=\"location.reload(); return false;\">Refresh</a>\n";
        ss << "    <br>&nbsp;&nbsp;&nbsp;&nbsp;<b>All Nodes' State and WatchPoint</b>\n";
        ss << "    <br>&nbsp;&nbsp;&nbsp;&nbsp;Total Nodes' Number: <b>" << node_num << "</b>\n";
        ss << "    <table frame=\"box\" rules=\"all\">\n";
        ss << "      <tr>\n";    
        ss << "        <td align=\"center\">Node</td>\n";
        ss << "        <td align=\"center\">State</td>\n";
        ss << "        <td align=\"center\">CPU Busy</td>\n";
        ss << "        <td align=\"center\">Load One</td>\n";
        ss << "      </tr>\n";
        for (int32_t j = 0; j < node_num; j++) {
            ss << "      <tr>";
            ss << "<td align=\"center\">"; 
            ss << cProtocol.protocolToStr((eprotocol)*p);
            p += 1;
            string ip = p;
            ss << ":" << ip.c_str();
            p += 24;
            ss << ":" << ntohs(*(uint16_t*)p) << "</td>";
            p += 2;
            estate state = (estate)(*p);
            p += 1;
            ss << "<td align=\"center\" bgcolor=\"";    
            if (state == normal) {        
                ss << "lawngreen\">";    
            } else {        
                ss << "red\">";    
            }
            if (state == normal) {
                ss << "Normal";
            } else if (state == abnormal) {
                ss << "Abnormal";
            } else if (state == timeout) {
                ss << "Timeout";
            } else if (state == unvalid) {
                ss << "Unvalid";
            } else if (state == uninit) {
                ss << "Uninit";
            } else {
                ss << "Unknown";
            }
            ss << "</td>";
            ss << "<td align=\"center\">" << ntohl(*(uint32_t*)p) << ".0 %</td>";
            p += 4;
            ss << "<td align=\"center\">" << ntohl(*(uint32_t*)p) << ".0</td>";
            p += 4;
            ss << "<tr>\n";
        }
        ss << "    </table>\n";
        ss << "  </body>\n";
        ss << "</html>\n";
    } else {
        ss << "Communication Error!";
        return -1;
    }
    return 0;
}



void sig_quit(int signum, siginfo_t* info, void* myact) {
    globalStopFlag = true;
}
int main(int argc, char *argv[]) {
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_flags=SA_SIGINFO;
    act.sa_sigaction=sig_quit;

    if(sigaction(SIGQUIT,&act,NULL)){
        printf("Sig error\n");
        return -1;
    }

    char log_name[128];
    sprintf(log_name, "WebServer_Client_%d.log", getpid());
    FILE* fp = fopen(log_name, "w");
    if(fp == NULL)
        return -1;
    fprintf(fp, "alog.rootLogger=INFO, A\n");
    fprintf(fp, "alog.appender.A=FileAppender\n");
    fprintf(fp, "alog.appender.A.fileName=%s\n", log_name);
    fprintf(fp, "alog.appender.A.flush=true\n");
    fclose(fp);
    alog::Configurator::configureLogger(log_name);

    if (argc < 2) {
        printf("%s clustermapserver(ip:tcp_port:udp_port,ip:tcp_port:udp_port) local_ip:local_port thread_num\n", argv[0]);
        return -1;
    }
    char* clustermap_server = argv[1];
    vector<string> map_server;
    char tmp[64];
    char* p = clustermap_server, *p1;
    while(p) {
        p1 = strchr(p, ',');
        if(p1) {
            memcpy(tmp, p, p1-p);
            tmp[p1-p] = 0;
            p = p1 + 1;
            map_server.push_back(tmp);
        } else {
            map_server.push_back(p);
            break;
        }
    }

    char *ip_port;
    string local_ip = "127.0.0.1";
    unsigned int port = 8000;
    if (argc >= 3) {
        ip_port = argv[2];
        p = ip_port;
        p1 = strchr(p, ':');
        if (strchr(ip_port, '.')) {
            if (p1) {
                memcpy(tmp, p, p1-p);
                tmp[p1-p] = 0;
                local_ip = tmp;
                p = p1 + 1;
            } else {
                local_ip = ip_port;
                p = " ";
            }             
        } else {
            if (p1) {
                p = p1 + 1;
            }
        }

        if (atoi(p) < 1) {
            fprintf(stdout, "port(%s) error! WebServer start at default port: 8000\n", p);
        } else {
            port = atoi(p);
        }
    } else {
        fprintf(stdout, "WebServer start at default port: 8000\n");
    }

    unsigned int num = 10;
    if (argc >= 4 && (num = atoi(argv[3])) < 1) {
        num = 10;
    }                
    string spec = "tcp:";
    spec += local_ip;
    spec += ":1";
    CMClient cClient;
    if(cClient.Initialize(spec.c_str(), map_server, NULL) != 0) {
        printf("WebServer Init Client failed\n");
        return -1;
    }
    if(cClient.Subscriber(1) != 0) {
        printf("WebServer subscriber failed\n");
        return -1;
    } 
    //    signal(SIGINT, singalHandler);
    //    signal(SIGTERM, singalHandler);

    int fd = open("isearch_cm_top.jpg", O_RDONLY);
    if ( -1 != fd ) {
        struct stat s;
        if (-1 != fstat(fd, &s) && S_ISREG(s.st_mode)) {
            size_logo = s.st_size;
            data_logo = (char*)mmap(0, size_logo, PROT_READ, MAP_PRIVATE, fd, 0);
        }           
    }               

    doProcess(port, num, cClient);
}
