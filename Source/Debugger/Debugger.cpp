#include "Debugger.h"
#include "imgui_internal.h"



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ImGuiWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // ImGuiにメッセージを渡す
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return TRUE;
    }

    // ImGuiが使わなかったメッセージは、0 を返してDxLibの標準処理に流す
    return FALSE;
}

//**************************************************************************************
//      * Debugger Class - コンストラクタ - *
//=======================================================================================
Debugger::Debugger()
{

}

//**************************************************************************************
//      * Debugger Class - デストラクタ - *
//=======================================================================================
Debugger::~Debugger()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

//**************************************************************************************
//      * Debugger Class - 初期化 - *
// 引数 1.ウインドウハンドル
//      2.描画管理クラスのポインタ
//=======================================================================================
bool Debugger::Setup(HWND hWnd, class ID3D11Device* device, class ID3D11DeviceContext* device_context)
{
    SetHookWinProc(ImGuiWndProc);	// ImGui用のウィンドウプロシージャを登録


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // キーボードの入力を有効化
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // ドッキングを有効化


    //ImGui::StyleColorsDark();    // デフォルト（暗め）
    ImGui::StyleColorsLight();   // 明るい色調
    //ImGui::StyleColorsClassic(); // 昔ながらのImGuiスタイル

    // 日本語フォント設定
    {
        // 日本語フォントのパスを指定（例：Windows標準のメイリオ）
        // C:/Windows/Fonts/msjh.ttc (微軟正黑體) や msgothic.ttc など
        ImFontConfig config;
        config.MergeMode = false; // 他のフォントと合成しない場合はfalse

        // 日本語の全範囲をカバーするグリフ（文字）範囲を指定
        const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesJapanese();

        // フォントをロード（サイズ18px）
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\meiryo.ttc", 20.0f, &config, glyphRanges);
    }

    // ImGuiのWindows用バックエンドとDirectX11用バックエンドの初期化をする
    if (!ImGui_ImplWin32_Init(hWnd))
        return false;
    if (!ImGui_ImplDX11_Init(device, device_context))
        return false;


    return true;
}


//**************************************************************************************
//      * Debugger Class - 描画前に呼ぶやつ - *
//=======================================================================================
void Debugger::BeginFrame(float winW, float winH, bool isEditorMode)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(winW, winH);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // ウィンドウ名や表示順に依存しない固定IDを使う。
    // IDを変えない限り、ユーザーが調整した配置をimgui.iniから復元できる。
    const ImGuiID dockspaceId = ImHashStr("MainEditorDockSpace");

    if (!isEditorMode)
    {
        // 通常プレイ中はドックを描画せず、ゲーム画面を従来どおり全画面表示する。
        // KeepAliveOnlyで内部ノードだけ維持し、編集モードへ戻したときに配置が崩れるのを防ぐ。
        if (ImGui::DockBuilderGetNode(dockspaceId) != nullptr)
        {
            ImGui::DockSpaceOverViewport(
                dockspaceId,
                viewport,
                ImGuiDockNodeFlags_KeepAliveOnly |
                ImGuiDockNodeFlags_PassthruCentralNode);
        }
        return;
    }

    // 保存済みレイアウトがない初回だけUnity風の初期配置を構築する。
    // 毎フレーム再構築すると、ユーザーによるドッキング変更がリセットされてしまう。
    if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
    {
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodePos(dockspaceId, viewport->WorkPos);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

        ImGuiID centerDockId = dockspaceId;
        ImGuiID hierarchyDockId = 0;
        ImGuiID inspectorDockId = 0;

        // まず左側にHierarchy領域を20%、残った領域の右側にInspector領域を24%確保する。
        // 分割後に残るcenterDockIdが、もっとも広い中央のGame領域になる。
        ImGui::DockBuilderSplitNode(
            centerDockId,
            ImGuiDir_Left,
            0.20f,
            &hierarchyDockId,
            &centerDockId);
        ImGui::DockBuilderSplitNode(
            centerDockId,
            ImGuiDir_Right,
            0.24f,
            &inspectorDockId,
            &centerDockId);

        // 日本語の表示名が変わっても配置を維持できるよう、各パネルは###以降の固定IDで紐付ける。
        ImGui::DockBuilderDockWindow("###Hierarchy", hierarchyDockId);
        ImGui::DockBuilderDockWindow("Game", centerDockId);
        ImGui::DockBuilderDockWindow("###Inspector", inspectorDockId);
        ImGui::DockBuilderFinish(dockspaceId);
    }

    // メインビューポート全体をドッキング可能領域として有効化する。
    ImGui::DockSpaceOverViewport(dockspaceId, viewport);
}


//**************************************************************************************
//      * Debugger Class - 描画後に呼ぶやつ - *
//=======================================================================================
void Debugger::EndFrame()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//**************************************************************************************
//      * Debugger Class - デバッグウインドウ - *
// 引数：ラベル名
//=======================================================================================
bool Debugger::BeginDebugWindow(const std::string& _label, ImGuiWindowFlags flags)
{
    ImGui::SetNextWindowBgAlpha(0.5f); // 背景の不透明度50%

    bool isOpen = true;
    return ImGui::Begin(
        _label.c_str(),
        &isOpen,
        flags
    );
}

//**************************************************************************************
//      * Debugger Class - デバッグウインドウの終了 - *
//     BeginDebugWindowを呼んだ後は必ず呼ぶ
//=======================================================================================
void Debugger::EndDebugWindow()
{
    ImGui::End();
}

//**************************************************************************************
//      * Debugger Class - テキスト表示- *
// 引数 1.テキスト
//=======================================================================================
void Debugger::DG_Text(std::string tex)
{
    ImGui::Text(tex.c_str());
}

void Debugger::DG_BulletTextValue(const std::string& text, const VECTOR& val)
{
    std::string combinedText = text + '\n';
    combinedText += "X : " + std::to_string(val.x) + "Y : " + std::to_string(val.y) + "Z : " + std::to_string(val.z);
    ImGui::BulletText(combinedText.c_str());
}

void Debugger::DG_BulletTextValue(const std::string& text, const int val)
{
    std::string combinedText = text + '\n';
    combinedText += " : " + std::to_string(val);
    ImGui::BulletText(combinedText.c_str());
}
void Debugger::DG_BulletTextValue(const std::string& text, const float val)
{
}
void Debugger::DG_BulletTextValue(const std::string& text, const double val)
{
}
void Debugger::DG_BulletTextValue(const std::string& text, const bool val)
{
    std::string combinedText = text + '\n';
    combinedText += val ? "true" : "false";
    ImGui::BulletText(combinedText.c_str());
}

//**************************************************************************************
//      * Debugger Class - この関数の上のテキストと下のテキストを繋げて表示するか - *
// 引数 1.繋げる際のオフセット開始位置
//=======================================================================================
void Debugger::DG_SameLine(float offsetStart_X)
{
    ImGui::SameLine(offsetStart_X);
}

//**************************************************************************************
//      * Debugger Class - カラーピッカー  - *
// 引数 1.ラベル名
//      2.カラーの格納先
//=======================================================================================
/****** RGB ******/
bool Debugger::DG_ColorPicker3(const std::string& label, float col[])
{
    return ImGui::ColorPicker3(label.c_str(), col);
}

bool Debugger::DG_ColorPicker3(const std::string& label, COLOR3* col3V)
{
    float col[3] = { col3V->r, col3V->g, col3V->b };

    bool res = ImGui::ColorPicker3(label.c_str(), col);
    col3V->r = col[0];
    col3V->g = col[1];
    col3V->b = col[2];
    return res;

}

/****** RGBA ******/
bool Debugger::DG_ColorPicker4(const std::string& label, float col[])
{
    return ImGui::ColorPicker4(label.c_str(), col);
}

bool Debugger::DG_ColorPicker4(const std::string& label, COLOR4* col4V)
{
    float col[4] = { col4V->r, col4V->g, col4V->b,col4V->a };

    bool res = ImGui::ColorPicker4(label.c_str(), col);
    col4V->r = col[0];
    col4V->g = col[1];
    col4V->b = col[2];
    col4V->a = col[3];
    return res;
}


//**************************************************************************************
//      * Debugger Class - カラーエディット - *
// 引数 1.ラベル名
//      2.カラーの格納先
//=======================================================================================
/****** RGB ******/
bool Debugger::DG_ColorEdit3(const std::string& label, float col[])
{
    return ImGui::ColorEdit3(label.c_str(), col);
}

bool Debugger::DG_ColorEdit3(const std::string& label, COLOR3* col3V)
{
    float col[3] = { col3V->r, col3V->g, col3V->b };

    bool res = ImGui::ColorEdit3(label.c_str(), col);
    col3V->r = col[0];
    col3V->g = col[1];
    col3V->b = col[2];
    return res;

}

/****** RGBA ******/
bool Debugger::DG_ColorEdit4(const std::string& label, float col[])
{
    return ImGui::ColorEdit4(label.c_str(), col);
}

bool Debugger::DG_ColorEdit4(const std::string& label, COLOR4* col4V)
{
    float col[4] = { col4V->r, col4V->g, col4V->b,col4V->a };

    bool res = ImGui::ColorEdit4(label.c_str(), col);
    col4V->r = col[0];
    col4V->g = col[1];
    col4V->b = col[2];
    col4V->a = col[3];
    return res;
}

//**************************************************************************************
//      * Debugger Class - int ドラッグして値を変えたり、直接入力で変えることも出来る - *
// 引数 1.ラベル名
//      2.値の数（４まで対応）  
//      3.値の参照＆格納先
//      4.値の変更スピード
//      5.最小値
//      6.最大値
//=======================================================================================
bool Debugger::DG_DragInt(const std::string& label, int valNum, int* val, float slideSpeed, int _min, int _max)
{
    switch (valNum)
    {
    case 1:
        return ImGui::DragInt(label.c_str(), val, slideSpeed, _min, _max);
        break;
    case 2:
        return ImGui::DragInt2(label.c_str(), val, slideSpeed, _min, _max);
        break;
    case 3:
        return ImGui::DragInt3(label.c_str(), val, slideSpeed, _min, _max);
        break;
    default:
        return false;
    }
    return false;
}

//**************************************************************************************
//      * Debugger Class - float ドラッグして値を変えたり、直接入力で変えることも出来る - *
// 引数 1.ラベル名
//      2.値の数（４まで対応）  
//      3.値の参照＆格納先
//      4.値の変更スピード
//      5.最小値
//      6.最大値
//=======================================================================================
bool Debugger::DG_DragFloat(const std::string& label, int valNum, float* val, float slideSpeed, float _min, float _max)
{
    switch (valNum)
    {
    case 1:
        return ImGui::DragFloat(label.c_str(), val, slideSpeed, _min, _max);
        break;
    case 2:
        return ImGui::DragFloat2(label.c_str(), val, slideSpeed, _min, _max);
        break;
    case 3:
        return ImGui::DragFloat3(label.c_str(), val, slideSpeed, _min, _max);
        break;
    default:
        return false;
    }
    return false;
}

//**************************************************************************************
//      * Debugger Class - VEC2 ドラッグして値を変えたり、直接入力で変えることも出来る - *
// 引数 1.ラベル名
//      2.値の参照＆格納先
//      3.値の変更スピード
//      4.最小値
//      5.最大値
//=======================================================================================
bool Debugger::DG_DragVec2(const std::string& label, VEC2* val, float slideSpeed, float _min, float _max)
{
    float temp[2] = { val->x, val->y };
    if (ImGui::DragFloat2(label.c_str(), temp, slideSpeed, _min, _max))
    {
        val->x = temp[0];
        val->y = temp[1];
        return true;
    }
    return false;
}


//**************************************************************************************
//      * Debugger Class - VEC3 ドラッグして値を変えたり、直接入力で変えることも出来る - *
// 引数 1.ラベル名
//      2.値の参照＆格納先
//      3.値の変更スピード
//      4.最小値
//      5.最大値
//=======================================================================================
bool Debugger::DG_DragVec3(const std::string& label, VECTOR* val, float slideSpeed, float _min, float _max)
{
    float temp[3] = { val->x, val->y,  val->z };
    if (ImGui::DragFloat3(label.c_str(), temp, slideSpeed, _min, _max))
    {
        val->x = temp[0];
        val->y = temp[1];
        val->z = temp[2];
        return true;
    }
    return false;
}



//**************************************************************************************
//      * Debugger Class - int スライダー - *
// 引数 1.ラベル名
//      2.値の数（４まで対応）  
//      3.値の参照＆格納先
//      4.最小値
//      5.最大値
//=======================================================================================
bool Debugger::DG_SliderInt(const std::string& label, int valNum, int* val, int _min, int _max)
{
    switch (valNum)
    {
    case 1:
        return ImGui::SliderInt(label.c_str(), val, _min, _max);
        break;
    case 2:
        return ImGui::SliderInt2(label.c_str(), val, _min, _max);
        break;
    case 3:
        return ImGui::SliderInt3(label.c_str(), val, _min, _max);
        break;
    case 4:
        return ImGui::SliderInt4(label.c_str(), val, _min, _max);
        break;
    default:
        return false;
    }
    return false;
}

//**************************************************************************************
//      * Debugger Class - float スライダー - *
// 引数 1.ラベル名
//      2.値の数（４まで対応）  
//      3.値の参照＆格納先
//      4.最小値
//      5.最大値
//=======================================================================================
bool Debugger::DG_SliderFloat(const std::string& label, int valNum, float* val, float _min, float _max)
{
    switch (valNum)
    {
    case 1:
        return ImGui::SliderFloat(label.c_str(), val, _min, _max);
        break;
    case 2:
        return ImGui::SliderFloat2(label.c_str(), val, _min, _max);
        break;
    case 3:
        return ImGui::SliderFloat3(label.c_str(), val, _min, _max);
        break;
    case 4:
        return ImGui::SliderFloat4(label.c_str(), val, _min, _max);
        break;
    default:
        return false;
    }
    return false;
}

//**************************************************************************************
//      * Debugger Class - float スライダー - *
// 引数 1.ラベル名
//      2.値の数（４まで対応）  
//      3.値の参照＆格納先
//      4.最小値
//      5.最大値
//=======================================================================================
bool Debugger::DG_SliderFloat(const std::string& label, VEC2* val, float _min, float _max)
{
    float temp[2] = { val->x,val->y };

    if (ImGui::SliderFloat2(label.c_str(), temp, _min, _max))
    {
        val->x = temp[0];
        val->y = temp[1];
        return true;
    }
    return false;
}

//**************************************************************************************
//      * Debugger Class - float スライダー - *
// 引数 1.ラベル名
//      2.値の数（４まで対応）  
//      3.値の参照＆格納先
//      4.最小値
//      5.最大値
//=======================================================================================
bool Debugger::DG_SliderFloat(const std::string& label, VECTOR* val, float _min, float _max)
{
    float temp[3] = { val->x,val->y,val->z };

    if (ImGui::SliderFloat3(label.c_str(), temp, _min, _max))
    {
        val->x = temp[0];
        val->y = temp[1];
        val->z = temp[2];
        return true;
    }
    return false;
}

//**************************************************************************************
//      * Debugger Class - ボタン - *
// 引数 1.ラベル名
//      2.大きさ 
// 
// 返り値：押されたかどうか
//=======================================================================================
bool Debugger::DG_Button(const std::string& label, VEC2 size)
{
    return ImGui::Button(label.c_str(), { size.x,size.y });
}


//**************************************************************************************
//      * Debugger Class - ラジオボタン 丸い奴 - *
// 引数 1.ラベル名
//      2.アクティブフラグ
// 
// 返り値：アクティブ状態かどうか 
//=======================================================================================
bool Debugger::DG_RadioButton(const std::string& label, bool active)
{
    return ImGui::RadioButton(label.c_str(), active);
}

//**************************************************************************************
//      * Debugger Class - チェックボックス - *
// 引数 1.ラベル名
//      2.アクティブフラグ 参照
// 
// 返り値：アクティブ状態かどうか 
//=======================================================================================
bool Debugger::DG_CheckBox(const std::string& label, bool* active)
{
    return ImGui::Checkbox(label.c_str(), active);
}

//*---------------------------------------------------------------------------------------
//*【?】画像表示
//*
//* [引数]
//* pSrv : 表示するテクスチャ(SRV)
//* size : サイズ（サイズに合わせて引き伸ばされる）
//* uv0 : 左上のUV座標
//* uv1 : 右下のUV座標
//*----------------------------------------------------------------------------------------
void Debugger::DG_Image(ID3D11ShaderResourceView* pSrv, VEC2 size, VEC2 uv0, VEC2 uv1)
{
    // 2. ImGuiの描画コード内で呼び出し
    if (pSrv)
    {
        // 画像のサイズを指定（例: 200x200ピクセル）
        ImVec2 imageSize = ImVec2(size.x, size.y);
        ImVec2 imageUV0 = ImVec2(uv0.x, uv0.y);
        ImVec2 imageUV1 = ImVec2(uv1.x, uv1.y);

        // SRVをImTextureIDにキャストして渡す
        ImGui::Image((ImTextureID)pSrv, imageSize, imageUV0, imageUV1);
    }
}

//*---------------------------------------------------------------------------------------
//*【?】文の最初の点みたいなやつ「・」←これ
//*
//* [引数]
//* なし
//*----------------------------------------------------------------------------------------
void Debugger::DG_Bullet()
{
    ImGui::Bullet();
}

//*---------------------------------------------------------------------------------------
//*【?】「・」が最初についたテキスト
//*
//* [引数]
//* text : 文字
//*----------------------------------------------------------------------------------------
void Debugger::DG_BulletText(const std::string& text)
{
    ImGui::BulletText(text.c_str());
}

//*---------------------------------------------------------------------------------------
//*【?】スペース
//*
//* [引数]
//* なし
//*----------------------------------------------------------------------------------------
void Debugger::DG_Spacing()
{
    ImGui::Spacing();
}

//*---------------------------------------------------------------------------------------
//*【?】区切り線
//*
//* [引数]
//* なし
//*----------------------------------------------------------------------------------------
void Debugger::DG_Separator()
{
    ImGui::Separator();
}

//*---------------------------------------------------------------------------------------
//*【?】項目をツリー上に表示
//*     エラーが出るので必ず、ifで確認する  
//*     最後に必ずTreePopを呼ぶ
//* 
//* [引数]
//* label : ツリーの見出し
//* 
//* [返値]
//* true : 成功
//* false : 失敗
//*----------------------------------------------------------------------------------------
bool Debugger::DG_TreeNode(const std::string& label)
{
    return ImGui::TreeNode(label.c_str());
}

//*---------------------------------------------------------------------------------------
//*【?】現在のインデント（下げ）を一つ戻し、
//*     そのツリー階層が終了したことを伝える 
//*
//* [引数]
//* なし
//*----------------------------------------------------------------------------------------
void Debugger::DG_TreePop()
{
    ImGui::TreePop();
}

//*---------------------------------------------------------------------------------------
//*【?】サイズ分、隙間を開けることができる
//*     改行みたいなことができる 
//*
//* [引数]
//* &_size : 隙間の大きさ
//*----------------------------------------------------------------------------------------
void Debugger::DG_Dummy(const VEC2& _size)
{
    ImGui::Dummy(ImVec2(_size.x, _size.y));
}

//*---------------------------------------------------------------------------------------
//*【?】
//*     
//*
//* [引数]
//* なし
//*----------------------------------------------------------------------------------------
bool Debugger::DG_Selectable(const std::string& _label, bool _selected, ImGuiSelectableFlags _flag, VEC2 _size)
{
    ImVec2 size = ImVec2(_size.x, _size.y);
    return ImGui::Selectable(_label.c_str(), &_selected, _flag, size);
}

//*---------------------------------------------------------------------------------------
//*【?】テクスチャの配列を持つテーブルを作る
//*     
//* [引数]
//* _label : テーブルのラベル
//* _columnCount : 列の数
//* _textures : テクスチャの配列
//* _srvArraySize : 配列のサイズ
//* _flags : テーブルのフラグ
//*----------------------------------------------------------------------------------------
void Debugger::DG_BeginTable(const std::string& _label, int _columnCount, TableTextures _textures[], int _srvArraySize, ImGuiTableFlags _flags)
{
    const float thumbnailSize = 200.0f;
    const float padding = 0.0f;

    if (ImGui::BeginTable("ImageTable", _columnCount))
    {
        for (int i = 0; i < _srvArraySize; ++i)
        {
            ImGui::TableNextColumn();

            // 画像
            ImGui::Image(
                _textures[i].pSrv,
                ImVec2(thumbnailSize, thumbnailSize)
            );

            // 名前
            ImGui::TextWrapped("%s", _textures[i].label.c_str());
        }

        ImGui::EndTable();
    }
}
