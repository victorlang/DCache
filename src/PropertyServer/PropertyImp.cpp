/**
* Tencent is pleased to support the open source community by making DCache available.
* Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
* Licensed under the BSD 3-Clause License (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of the License at
*
* https://opensource.org/licenses/BSD-3-Clause
*
* Unless required by applicable law or agreed to in writing, software distributed under
* the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
* either express or implied. See the License for the specific language governing permissions
* and limitations under the License.
*/
#include "PropertyImp.h"
#include "PropertyServer.h"

///////////////////////////////////////////////////////////
//

PropertyImp::PropertyImp()
{
}

void PropertyImp::initialize()
{

}

int PropertyImp::reportPropMsg(const map<DCache::StatPropMsgHead, DCache::StatPropMsgBody> &propMsg, tars::TarsCurrentPtr current)
{
    TLOGINFO("PropertyImp::reportPropMsg size:" << propMsg.size() << endl);

    for (map<DCache::StatPropMsgHead, DCache::StatPropMsgBody>::const_iterator it = propMsg.begin(); it != propMsg.end(); ++it)
    {
        const DCache::StatPropMsgHead &head = it->first;
        const DCache::StatPropMsgBody &body = it->second;

        if (body.vInfo.size() == 0)
        {
            continue;
        }

        dump2file();

        PropertyHashMap &propHashMap = g_app.getHashMap();
        float rate = (propHashMap.getMapHead()._iUsedChunk) * 1.0/propHashMap.allBlockChunkCount();

        if (rate > 0.9)
        {
            propHashMap.expand(propHashMap.getMapHead()._iMemSize * 2);
            TLOGERROR("PropertyImp::reportPropMsg hashmap expand to " << propHashMap.getMapHead()._iMemSize << endl);
        }

        PropHead tHead;
        vector<string> v = TC_Common::sepstr<string>(head.propertyName, ".");
        string sPropertyName;
        if (v.size() == 3)
        {
            sPropertyName = v[2];
        }
        else if (v.size() == 4)
        {
            sPropertyName = v[2] + "." + v[3];
        }
        else
        {
            sPropertyName = head.propertyName;
        }
        
        const map<string, string> &mPropertyName = g_app.getPropertyNameMap();
        map<string, string>::const_iterator itr = mPropertyName.find(sPropertyName);
        if (itr == mPropertyName.end())
        {
            ostringstream os;
            os.str("");
            head.displaySimple(os);
            TLOGERROR("PropertyImp::reportPropMsg invalid property:" << sPropertyName << "|head:" << os.str() << endl);
            
            continue;
        }

        //在此替换为插入DB的字段名
        tHead.moduleName    = head.moduleName;
        tHead.propertyName  = itr->second;
        tHead.setName       = head.setName;
        tHead.setArea       = head.setArea;
        tHead.setID         = head.setID;
        tHead.ip            = current->getIp();

        int iRet = propHashMap.add(tHead, body);
        if (iRet != TC_HashMap::RT_OK)
        {
            TLOGERROR("PropertyImp::reportPropMsg add hashmap record return:" << iRet << endl);
        }
            
        if (LOG->IsNeedLog(TarsRollLogger::INFO_LOG))
        {
            ostringstream os;
            os.str("");
            head.displaySimple(os);
            body.displaySimple(os);
            TLOGINFO("PropertyImp::reportPropMsg |" << iRet << "|" << os.str() << endl);
        }
    }
    
    return 0;
}

void PropertyImp::dump2file()
{
    static string g_sDate;
    static string g_sFlag;

    time_t tNow			= TNOW;
    time_t tInsertIntev = g_app.getInsertInterval() * 60; // second

    static time_t g_tLastDumpTime = 0;

    if (g_tLastDumpTime == 0)
    {
        g_app.getTimeInfo(g_tLastDumpTime, g_sDate, g_sFlag);
    }

    if (tNow - g_tLastDumpTime > tInsertIntev)
    {
        static TC_ThreadLock g_mutex;
        TC_ThreadLock::Lock lock(g_mutex);
        if (tNow - g_tLastDumpTime > tInsertIntev)
        {
            g_app.getTimeInfo(g_tLastDumpTime, g_sDate, g_sFlag);
            string sFile = g_app.getClonePath() + "/" + g_sDate + g_sFlag + ".txt";
            int iRet = g_app.getHashMap().dump2file(sFile, true);
            if(iRet != 0)
            {
                TC_File::removeFile(sFile, false);
                TLOGERROR("PropertyImp::dump2file |" << iRet << "|" << sFile << endl);
            }
            else
            {
                TLOGDEBUG("PropertyImp::dump2file |" << sFile << endl);
            }
        }
    }
}
