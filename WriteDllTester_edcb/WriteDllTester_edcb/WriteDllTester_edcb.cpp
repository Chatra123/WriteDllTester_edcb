/*

based on [ xtne6f/EDCB work-plus-s 160616 ]


command line               [1]           [2]            [3]               [4]           [5]      [6]
  WriteDllTester_edcb.exe  "input_path"  "save_folder"  "save_foldersub"  limit_MiBsec  dllname  overwrite


デバッグ  -->  引数
　args[1]              [2]              [3]                 [4]           [5]                [6]
  "input_file"         "output_folder"  "output_foldersub"  limit_MiBsec  dll_name           overwrite
  "D:\input.ts"        "D:\\"           "E:\\"              0             Write_Default.dll  1
  "D:\input.ts"        "D:\rec\\"       "E:\\"              2.0
  "E:\TS_Samp\t2s.ts"  "E:\pf_out"      "D:"                20.0          Write_PF.dll       1


  ・出力フォルダが存在しないと作成される。
  ・フォルダ名の最後に\はつけなくてもいい。
  ・c++  最後に\をつける場合は\\にする。\だけだと特殊文字 \"と認識される。
  ・引数間に全角スペースがあるとダメ
*/



// WriteDllTester_edcb.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include "Shlwapi.h"
#pragma comment(lib, "shlwapi.lib")

using namespace std;
using namespace std::chrono;

#include "../../BonCtrl/WriteTSFile.h"




//==================================
// func
//==================================
bool CheckDLL(wstring dllname);
void ReadSpeed_CrackDown(double limit, __int64 read);


//==================================
// main
//==================================
int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
  ///*  test args  */
  //wchar_t *test_argv[16] = {};
  //test_argv[0] = L"app_path";
  //test_argv[1] = L"E:\\TS_Samp\\t2s.ts";     // input path
  //test_argv[2] = L"D:\\";                    // save_folder
  //test_argv[3] = L"none foldersub";          // save_foldersub
  //test_argv[4] = L"-1";                      // limit
  //test_argv[5] = L"Write_Default.dll";       // dllname
  //test_argv[6] = L"1";                       // overwrite
  //argc = 7;
  //argv = test_argv;


  //setting
  wstring input_file, save_folder, save_foldersub;
  double limit_MiB = 0;
  wstring dllname;
  bool overwrite = true;


  /*  test setting  */
  //input_file = L"E:\\TS_Samp\\t60s.ts";
  //input_file = L"E:\\TS_Samp\\t2s.ts";
  //save_folder = L"";
  //save_folder = L"D:\\";
  //save_foldersub = L"E:\\";
  //limit_MiB = 1.7;
  //limit_MiB = 20.0;
  //limit_MiB = 0;
  //dllname = L"Write_PF.dll";
  //dllname = L"Write_Default.dll";
  //overwrite = true;


  //setting from commandline
  if (2 <= argc) input_file = argv[1];
  if (3 <= argc) save_folder = argv[2];
  if (4 <= argc) save_foldersub = argv[3];
  if (5 <= argc)
  {
    if ((_snwscanf_s(argv[4], 6, L"%lf", &limit_MiB)) <= 0)
      limit_MiB = 0;
  }
  if (6 <= argc) dllname = argv[5];
  if (7 <= argc)
  {
    /*
      overwrite = argv[6] == L"1";だと正しく処理できない。
      wchar_t *  -->  wstring
    */
    overwrite = wstring(argv[6]) == L"1";
  }

  //show setting
  wcerr << fixed << setprecision(1);       //小数点１桁まで表示
  wcerr << endl;
  wcerr << L"  input_file     : " << input_file << endl;
  wcerr << L"  save_folder    : " << save_folder << endl;
  wcerr << L"  save_foldersub : " << save_foldersub << endl;
  wcerr << L"  limit MiB/sec  : " << limit_MiB << endl;
  wcerr << L"  dllname        : " << dllname << endl;
  wcerr << L"  overwrite      : " << overwrite << endl;
  wcerr << L"--------------------------------------------" << endl;
  wcerr << endl;





  //初期化
  //dll
  bool checkOK = CheckDLL(dllname);
  if (checkOK == false)
  {
    Sleep(2000);
    return 1;
  }

  //input
  ifstream ifs(input_file, std::ios_base::in | std::ios_base::binary);
  if (!ifs) {
    wcerr << endl;
    wcerr << L"fail to open file" << endl;
    Sleep(2000);
    return 1;
  }

  //output
  CWriteTSFile writeTSFile;
  {
    wstring filename = PathFindFileName(input_file.c_str());

    vector<REC_FILE_SET_INFO> saveFolder;
    REC_FILE_SET_INFO folderItem;
    {
      folderItem.recFolder = save_folder;
      folderItem.writePlugIn = dllname;
      folderItem.recNamePlugIn = L"";
      folderItem.recFileName = L"";
    }
    saveFolder.emplace_back(folderItem);

    vector<wstring> saveFolderSub;
    {
      saveFolderSub.emplace_back(save_foldersub);
    }

    //start save
    const ULONGLONG WriteBuffSize = 770448;
    BOOL canSave = writeTSFile.StartSave(filename, overwrite, WriteBuffSize, &saveFolder, &saveFolderSub, 770048);
    if (canSave == FALSE)
    {
      Sleep(2000);
      return 1;
    }
  }



  //
  // main loop
  //
  const int chunk = 188 * 5;
  double limit = limit_MiB * 1024 * 1024;

  __int64 TotalRead = 0;
  __int64 ErrorCounter = 0;
  while (true)
  {
    //進捗
    if (TotalRead % (chunk * 50 * 1024) == 0)  // tick tock 50 MB
    {
      int read_MiB = (int)(1.0 * TotalRead / 1024 / 1024);
      //wcerr << L"  tester read  " << std::setw(4) << read_MiB << "  MiB" << endl;
    }

    //読
    char data[chunk] = {};
    DWORD read = (DWORD)ifs.read((char *)data, sizeof(char) *chunk).gcount();
    TotalRead += read;
    if (read == 0 && ifs.eof())
      break;
    //速度制限
    ReadSpeed_CrackDown(limit, read);


    //書
    BOOL success = writeTSFile.AddTSBuff((BYTE *)&data, read);
    if (success == FALSE)
    {
      ErrorCounter++;
      __int64 writeSize = 0;
      writeTSFile.GetRecWriteSize(&writeSize);
      wcerr << endl;
      wcerr << L"AddTSBuff error :" << endl;
      wcerr << L"    TotalRead  = " << TotalRead << endl;
      wcerr << L"    TotalWrite = " << writeSize << endl;
    }

  }//while
  writeTSFile.EndSave();



  //show result
  __int64 writeSize = 0;
  writeTSFile.GetRecWriteSize(&writeSize);
  wcerr << endl;
  wcerr << L"--------------------------------------------" << endl;
  wcerr << L"tester" << endl;
  wcerr << L"  TotalRead     =  " << TotalRead << endl;
  wcerr << L"  TotalWrite    =  " << writeSize << endl;
  wcerr << L"  ErrorCounter  =  " << ErrorCounter << endl;
  wcerr << endl;
  wcerr << endl;
  wcerr << L"  finish" << endl;
  Sleep(4 * 1000);

  return 0;
}






//==================================
// CheckDLL
//==================================
bool CheckDLL(wstring dllname)
{
  wstring writeFolder;
  {
    wchar_t exePath[_MAX_PATH];
    GetModuleFileName(NULL, exePath, sizeof(exePath));
    wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR];
    _tsplitpath_s(exePath, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, NULL, NULL, NULL);
    writeFolder = wstring(drive) + wstring(dir) + L"Write\\";
  }
  CreateDirectory(writeFolder.c_str(), NULL);


  wstring dllpath = writeFolder + dllname;
  ifstream ifs(dllpath, std::ios_base::in | std::ios_base::binary);
  if (!ifs) {
    wcerr << endl;
    wcerr << L"dll not found" << endl;
    return false;
  }


  CWritePlugInUtil util;
  BOOL success = util.Initialize(dllpath.c_str());
  if (success)
  {
    return true;
  }
  else
  {
    wcerr << endl;
    wcerr << L"fail to initialize dll" << endl;
    return false;
  }
}


//==================================
// 速度制限
//==================================
auto tickBeginTime = system_clock::now();        //速度制限    計測開始時間
__int64 tickReadSize = 0;                        //速度制限    200ms間の読込み量
void ReadSpeed_CrackDown(double limit, __int64 read)
{
  if (limit <= 0) return;

  tickReadSize += read;

  auto tickElapse = system_clock::now() - tickBeginTime;
  auto elapse_ms = duration_cast<milliseconds>(tickElapse).count();

  //200msごとにカウンタリセット
  if (200 <= elapse_ms)
  {
    tickBeginTime = system_clock::now();
    tickReadSize = 0;
  }

  //読込量が制限をこえたらsleep_for
  if (limit * (200.0 / 1000.0) < tickReadSize)
    this_thread::sleep_for(milliseconds(200 - elapse_ms));

}



//==================================
// OutputDebugStringをコンソールに表示
//==================================
void OutputDebugStringWrapper(LPCWSTR lpOutputString)
{
  wcerr << lpOutputString << endl;
}














