﻿
; ANIMATOON 
; PB5.22 - 5.31
; by Blendman
; Date 06/2015 




;--- FILE
Macro WinDocNewCont(typ)
  
  If IsGadget(cont) And cont > #G_LastGadget
    FreeGadget(cont)
  EndIf
  
  Cont = ContainerGadget(#PB_Any,xx,yy,w1-GadgetWidth(canvas)-20,h1-50)
  SetGadgetColor(Cont,#PB_Gadget_BackColor,col2)
  u = xx
  xx = 10
  
  Select Typ
      
    Case 0 ; recent document
      
    Case 1 ; template "classique"
      If cont
        
        nomtxt=TG(xx,yy,"Name",col2)        
        x1 = xx + GadgetWidth(nomtxt)
        w2 = w1-x1-u-20
        SGnom  = StringGadget(#PB_Any, x1,yy,w2+10,20,"Untitled")
        yy+25
        w2 = w1-u-10
        FrameGadget(#GADGET_WNewTile,xx,yy,w2-10,150,"Image size")
        SetGadgetColor(#GADGET_WNewTile,#PB_Gadget_BackColor,col2)
        yy + 20
        TGTemplate = TextGadget(#PB_Any,xx+5,yy,Len(lang("Templates")+":")*7,20,lang("Templates")+":")
        SetGadgetColor(TGTemplate,#PB_Gadget_BackColor,col2)
        xt = GadgetWidth(TGTemplate)+5
        CbTemplate = ComboBoxGadget(#PB_Any,xx+xt,yy,w2-xt-20,20) : yy + 40
        
        
        
        
        Directory$ = GetCurrentDirectory() + "data\Presets\Template\"  ; Liste tous les fichiers et les dossiers du répertoire racine de l'utilisateur qui est actuellement logué (Home)
        Dim template.sTemplate(0)
        If ExamineDirectory(0, Directory$, "*.txt")  
          While NextDirectoryEntry(0)
            If DirectoryEntryType(0) = #PB_DirectoryEntry_File
              
              template_nom$ = DirectoryEntryName(0)
              If ReadFile(0, Directory$+template_nom$)                
                While Eof(0) = 0       
                  info$ = ReadString(0) 
                  ReDim template.sTemplate(i)
                  
                  template(i)\nom$ = StringField(info$,1,"|")
                  template(i)\dpi = Val(StringField(info$,2,"|"))
                  template(i)\format$ = StringField(info$,3,"|")
                  template(i)\w = Val(StringField(info$,4,"|"))
                  template(i)\h = Val(StringField(info$,5,"|"))
                  
                  AddGadgetItem(CbTemplate,i, template(i)\nom$)
                  i+1
                Wend
                ReDim template.sTemplate(i-1)
                CloseFile(0) 
              EndIf
            EndIf
          Wend
          FinishDirectory(0)
        EndIf
        
        n=7
        Dim Format$(n)
        Format$(0) = "Millimeters (mm)"
        Format$(1) = "Centimeters (cm)"
        Format$(2) = "Decimeters (dm)"
        Format$(3) = "Inch (in)"
        Format$(4) = "Pica (pi)"
        Format$(5) = "Cicero (cc)"
        Format$(6) = "Points (pt)"
        Format$(7) = "Pixels (px)"
        
        TGWidth=TG(xx+5,yy, lang("Width"),col2)
        SpinGadget(#GADGET_WNewW,xx+60,yy,50,20,1,10000,#PB_String_Numeric) : SetGadgetText(#GADGET_WNewW,Str(doc\w_def))
        CBPixelW = ComboBoxGadget(#PB_Any,xx+115,yy,80,20) 
        
        YY +25
        TGHeight=TG(xx+5,yy, lang("Height"),col2)
        SpinGadget(#GADGET_WNewH,xx+60,yy,50,20,1,10000,#PB_String_Numeric) : SetGadgetText(#GADGET_WNewH,Str(doc\h_def)) 
        CBPixelH = ComboBoxGadget(#PB_Any,xx+115,yy,80,20) 
        
        For i = 0 To n
          AddGadgetItem(CBPixelH,i,Format$(i))
          AddGadgetItem(CBPixelW,i,Format$(i))
        Next i
        SetGadgetState(CBPixelH,n)
        SetGadgetState(CBPixelW,n)
        
        YY -12.5
        x1 = xx+5+GadgetWidth(TGHeight)+GadgetWidth(#GADGET_WNewH)+GadgetWidth(CBPixelH)+40
        TGReso = TG(x1,yy,"Resolution",col2)
        SGReso = SpinGadget(#PB_Any,x1+GadgetWidth(TGReso)-15,yy,50,20,1,10000,#PB_String_Numeric) : SetGadgetText(SGReso,"100")
        
        yy +25+12.5
        
        
        CloseGadgetList()
      EndIf
      
    Case 2 ; preset cinema 
      
  EndSelect
  
  
  
EndMacro
Procedure WindowDocNew()
  
  If OpenWindow(#winNewTileset,0,0, 650,500,Lang("NewDoc"),#PB_Window_ScreenCentered|#PB_Window_SystemMenu, WindowID(#WinMain))
    
    w1 = WindowWidth(#winNewTileset)
    h1 = WindowHeight(#winNewTileset)
    Col = RGB(80,80,80)
    col2 = RGB(120,120,120)
    SetWindowColor(#winNewTileset,col)
    
    xx = 165
    canvas = CanvasGadget(#PB_Any,10,10,150,WindowHeight(#winNewTileset)-50) 
    If StartDrawing(CanvasOutput(canvas))
      Box(0,0,200,1000,col2)
      StopDrawing()
    EndIf
    yy=10
    
    WinDocNewCont(1)
    
    ButtonGadget(#GADGET_WNewBtnOk,w1-70,h1 -25,60,20,Lang("Ok"))
    BtnCancel = ButtonGadget(#PB_Any,10,h1-25,GadgetWidth(canvas),20,Lang("Cancel"))
    SetGadgetColor(BtnCancel,#PB_Gadget_BackColor,col2)
    SetGadgetColor(#GADGET_WNewBtnOk,#PB_Gadget_BackColor,col2)
    
    Repeat
      
      Event       = WaitWindowEvent(10)
      EventGadget = EventGadget()
      EventType   = EventType()
      
      Select event
          
          
        Case #PB_Event_Gadget
          
          Select EventGadget
              
            Case CbTemplate
              templat = GetGadgetState(CbTemplate)
              SetGadgetText(#GADGET_WNewW,Str(template(templat)\w))
              SetGadgetText(#GADGET_WNewH,Str(template(templat)\h))
              
            Case canvas
              
            Case SGnom
              name$ =GetGadgetText(SGNom)
              
              
              
            Case #GADGET_WNewW,#GADGET_WNewH
              W = Val(GetGadgetText(#GADGET_WNewW))
              If W <1
                SetGadgetText(#GADGET_WNewW,Str(1))
              ElseIf W > 10000
                SetGadgetText(#GADGET_WNewW,Str(10000))
              EndIf
              H = Val(GetGadgetText(#GADGET_WNewH))
              If H <1 
                SetGadgetText(#GADGET_WNewH,Str(1))
              ElseIf H > 10000
                SetGadgetText(#GADGET_WNewH,Str(10000))
              EndIf
              
            Case #GADGET_WNewBtnOk
              quit = 1
              ok = 1
              
            Case BtnCancel              
              quit = 1
              
          EndSelect
          
          
        Case #PB_Event_CloseWindow
          quit = 1
          
      EndSelect
      
      
      
      
    Until quit = 1
    
    
    If ok =1
      
      ; we have clicked on ok  button, we can create a new document
      Layer_FreeAll()
      
      
      
      ; then, set the new document parameters
      Doc\name$ = name$
      Doc\w = Val(GetGadgetText(#GADGET_WNewW))
      Doc\h = Val(GetGadgetText(#GADGET_WNewH))
      
      ; I have to delete the sprite layer tempo (the sprite for temporary operations) and other sprite and re create it (because it has new size)
      RecreateLayerUtilities()

      
      ; add a new layer.
      Layer_Add()
      
      ; reset the options parameters
      OptionsIE\AutosaveFileName$ = ""
      OptionsIE\NbNewFile +1
      
      
      ;ClearGadgetItems(#G_LayerList)
      ;n = ArraySize(layer())
      ;AddGadgetItem(#G_LayerList,-1,Layer(layerId)\Name$)
      
      ; update the GUI
      IE_StatusBarUpdate()
      ScreenUpdate()
      
    EndIf
    
    
    FreeArray(template())
    FreeArray(Format$())
    
    CloseWindow(#winNewTileset)
    
    
  EndIf  
  
EndProcedure

Procedure WindowPref()
  
  Shared animBarre.a
  
  If OpenWindow(#Win_Pref,0,0,400,300,Lang("Preferences"),#PB_Window_ScreenCentered|#PB_Window_SystemMenu)
    
    If PanelGadget(#G_PrefPanel,5,5,390,290)
      
      ;{ General
      AddGadgetItem(#G_PrefPanel,0,lang("General"))
      
      FrameGadget(#G_Frame_Lang,5,5,100,40,lang("Langage"))
      
      ComboBoxGadget(#G_Cob_Lang,15,20,80,20)
      
      Directory$ = "data\Lang\"   
      If ExamineDirectory(0, Directory$, "*.ini")  
        While NextDirectoryEntry(0)
          If DirectoryEntryType(0) = #PB_DirectoryEntry_File
            nom$ = RemoveString(DirectoryEntryName(0),".ini")            
            If nom$ = OptionsIE\Lang$
              pos = CountGadgetItems(#G_Cob_Lang)
            EndIf
            AddGadgetItem(#G_Cob_Lang,-1,nom$)  
          EndIf
        Wend
        FinishDirectory(0)
      EndIf
      SetGadgetState(#G_Cob_Lang,pos)
      ;}
      
      ;{ Grille
      
      AddGadgetItem(#G_PrefPanel,1,lang("Grid"))
      SpinGadget(#G_GridW,5,10,50,20,1,500,#PB_Spin_Numeric)
      SpinGadget(#G_GridH,60,10,50,20,1,500,#PB_Spin_Numeric)
      ButtonGadget(#G_GridColor,120,10,60,20,lang("Grid Color"))
      SetGadgetState(#G_GridW,OptionsIE\GridW)
      SetGadgetState(#G_GridH,OptionsIE\gridH)     
      ;}
      
      ;{ Preset
      AddGadgetItem(#G_PrefPanel,-1,lang("Brush Preset"))
      CheckBoxGadget(#G_BrushPreset_Save_color,10,10,Len(Lang("SaveColorBrush"))*6,20,Lang("SaveColorBrush"))
      ;}
      
      ;{ Animation
      AddGadgetItem(#G_PrefPanel,-1,Lang("Animation"))
      
      ComboBoxGadget(#G_WAnim_CoBFrameTimeline,10,10,80,20)
      AddGadgetItem(#G_WAnim_CoBFrameTimeline,-1,lang("Frame"))
      AddGadgetItem(#G_WAnim_CoBFrameTimeline,-1,lang("Seconde"))
      SetGadgetState(#G_WAnim_CoBFrameTimeline,0)
      
      ComboBoxGadget(#G_WAnim_CBTimelineBar, 10,40,80,20)
      AddGadgetItem(#G_WAnim_CBTimelineBar,-1,lang("Barre"))
      AddGadgetItem(#G_WAnim_CBTimelineBar,-1,lang("Line"))
      SetGadgetState(#G_WAnim_CBTimelineBar,animBarre)
      
      SpinGadget(#G_WPref_SizeFrame,10,80,30,20,4,24,#PB_Spin_Numeric)
      SetGadgetState(#G_WPref_SizeFrame,OptionsIE\SizeFrameW)
      
      ;}
      
      
      CloseGadgetList()      
    EndIf
    
  EndIf
  
EndProcedure


;--- EDITIONS


;--- IMAGES



;--- VIEW



;--- LAYERS
Macro WinLayerPropCont(newtyp)
  
  If Typ <> newtyp
    
    Typ = newtyp
    
    If IsGadget(cont) And cont > #G_LastGadget
      FreeGadget(cont)
    EndIf
    
    xx = 160 : yy = 5
    www = w1-GadgetWidth(canvas)-GadgetWidth(btnok)-30
    hhh = h1-50
    If hhh < 50
      hhh = WindowHeight(#Win_Layer) - 20
    EndIf
    If www < 50
      www = 300
    EndIf  
    Cont = ContainerGadget(#PB_Any,xx,yy,www,hhh)
    
    ; SetGadgetColor(Cont,#PB_Gadget_BackColor,col2)
    u = xx
    xx = 10
    w3 = www-10
    
    Select Typ
        
      Case 0 ; general
        If cont           
          
          nomtxt=TG(xx,yy,Lang("Name"))        
          x1 = xx + GadgetWidth(nomtxt)
          w3 -x1
          SGnom  = SG(x1,yy,w3+10,layer(layerid)\Name$)
          yy+25
          w3 = www
          FrameGadget(#GADGET_WNewTile,xx,yy,w3-10,150,Lang("General"))
          ; SetGadgetColor(#GADGET_WNewTile,#PB_Gadget_BackColor,col2)
          yy + 20
          
          CloseGadgetList()
        EndIf
        
      Case 1 ; style 
        If cont  
          FrameGadget(#GADGET_WNewTile,xx,yy,w3-10,150,Lang("Style"))
          CloseGadgetList()
        EndIf
        
      Case 2 ; drop shadow
        If cont 
          FrameGadget(#GADGET_WNewTile,xx,yy,w3-10,150,Lang("Drop shadow"))
          CloseGadgetList()
        EndIf
        
      Case 3 ; inner shadow
        If cont 
          FrameGadget(#GADGET_WNewTile,xx,yy,w3-10,150,Lang("Inner shadow"))
          CloseGadgetList()
        EndIf
        
      Case 4 ; outer glow
        If cont  
          FrameGadget(#GADGET_WNewTile,xx,yy,w3-10,150,Lang("Outer glow"))
          CloseGadgetList()
        EndIf
        
      Case 5 ; inner glo<w
        If cont  
          FrameGadget(#GADGET_WNewTile,xx,yy,w3-10,150,Lang("Inner glow"))
          CloseGadgetList()
        EndIf
        
      Case 6 ; border
        If cont  
          FrameGadget(#GADGET_WNewTile,xx,yy,w3-10,150,Lang("Border"))
          CloseGadgetList()
        EndIf
        
    EndSelect
    
  EndIf
  
EndMacro
Procedure WindowLayerProp()
  
  
  If OpenWindow(#Win_Layer,0,0,600,400,Lang("Layer Properties"),#PB_Window_SystemMenu|#PB_Window_ScreenCentered,WindowID(#WinMain))
    
    
    ww = 150
    h1 = WindowHeight(#Win_Layer)
    w1 = WindowWidth(#Win_Layer)
    canvas = ContainerGadget(#PB_Any,5,5,ww,h1-20,#PB_Container_Flat)
    If canvas
      
      xx = 5
      yy = 5
      w2 = ww-10
      
      CB_General    = AddButon2(-2,-1,w2+15,26,"  "+ Lang("General"),#PB_Button_Left,"") : yy+21
      CB_Style      = AddButon2(-2,yy,w2+15,26,"  "+ Lang("Style"),#PB_Button_Left,"") : yy+27
      CB_DropShado  = AddCheckBox2(xx,yy,w2,20,Lang("Drop shadow"),0,"") : yy+20
      CB_InShado    = AddCheckBox2(xx,yy,w2,20,Lang("Inner shadow"),0,"") : yy+20
      CB_OutGlow    = AddCheckBox2(xx,yy,w2,20,Lang("Outer glow"),0,"") : yy+20
      CB_InGlow     = AddCheckBox2(xx,yy,w2,20,Lang("Inner glow"),0,"") : yy+20
      CB_Stroke     = AddCheckBox2(xx,yy,w2,20,Lang("Border"),0,"") : yy+20
      
      CloseGadgetList()
    EndIf
    
    yy = 5
    wu = 80
    btnok     = AddButon2(w1-wu-5,yy,wu,20,Lang("Ok"),#PB_Button_Default,"")      : yy+21
    btnCancel = AddButon2(w1-wu-5,yy,wu,20,Lang("Cancel"),#PB_Button_Default,"")  : yy+21
    btnNewStyle = AddButon2(w1-wu-5,yy,wu,20,Lang("New Style"),#PB_Button_Default,"")  : yy+21
    
    yy+5
    ; preview style 
    img_prev_style = CreateImage(#PB_Any, wu,wu)
    If StartDrawing(ImageOutput(img_prev_style))
      Box(0,0,wu,wu,RGB(190,190,190))
      Box(wu/2-10,wu/2-10,20,20,RGB(120,120,120))
      StopDrawing()
    EndIf
    Preview   = ImageGadget(#PB_Any, w1-wu-5,yy,wu,wu,ImageID(img_prev_style))
    
    Typ = -1
    
    WinLayerPropCont(0)
    
    
    Select layer(LayerId)\Typ
        
      Case #Layer_TypText
        ; Layer_ChangeText()                       
        
    EndSelect  
    
    
    
    Repeat
      
      event = WaitWindowEvent(10)
      
      Select event
          
          
        Case #PB_Event_Gadget
          Select EventGadget()
              
            Case CB_Style
              WinLayerPropCont(1)
              
            Case CB_General
              WinLayerPropCont(0)
              
            Case  CB_DropShado 
              WinLayerPropCont(2)
              
            Case  CB_InShado 
              WinLayerPropCont(3)
              
            Case  CB_OutGlow 
              WinLayerPropCont(4) 
              
            Case  CB_InGlow 
              WinLayerPropCont(5)
              
            Case  CB_Stroke 
              WinLayerPropCont(6)
              
            Case btnok
              quit = 1
              ok = 1
              
            Case btnCancel
              quit = 1 
              
            Case btnNewStyle
              
              
              
          EndSelect
          
        Case #PB_Event_CloseWindow
          quit = 1
          
      EndSelect
      
    Until quit = 1
    
    
    CloseWindow(#Win_Layer)
    freeimage2(img_prev_style)
    
    If ok = 1
    EndIf
    
    
  EndIf
  
  
  
  
  
EndProcedure


; Paper
Procedure UpdateCanvasBGColor(x,y,wcol,image)
  
  ; Then update the canvasBGcolor
  If StartDrawing(CanvasOutput(#GADGET_WinBGED_Canvas_colors))
    ; draw a color on the canvas
    Box(0, 0, OutputWidth(), OutputHeight(), col2)
    
    ; then draw the colors from the colorfile
    If IsImage(image)
      DrawingMode(#PB_2DDrawing_AlphaBlend)
      DrawAlphaImage(ImageID(image), 0, 0, 255)
    Else
      Debug "error image BGcolor in window.pbi"
    EndIf
    
    ; then draw the selected color
    DrawingMode(#PB_2DDrawing_Outlined )
    Box(x,y,wcol,wcol, RGB(255,0,0))
    Box(x+1,y+1,wcol-2,wcol-2, RGB(0,0,0))
    
    StopDrawing()
  EndIf
  
EndProcedure
Procedure UpdateIMageBGColor(image, filecolor$, wcol)
  
  ; to update the color on the canvas BGcolor(background editor)
  If StartDrawing(ImageOutput(image))
    
    
    
    ; then draw the colors from the colorfile
    ;{ open the bgcolors and draw it
    
    ; width for the colors case
    wcanvas = ImageWidth(image)
    
    ; Debug filecolor$
    DrawingMode(#PB_2DDrawing_AllChannels)
   
    ; check if file exists
    If GetFileExist(filecolor$)
      
      ; and read the file
      If ReadFile(0, filecolor$)
        j = 0
        While Eof(0) = 0    
          
          line$ = ReadString(0)
          char$ = Mid(line$,1,1)
          
          If char$ <> "#" And line$ <> ""            
            
            line$ = ReplaceString(line$,Chr(9),Chr(32)) 
            
            While char$ = Chr(32)
              char$ = Mid(line$,1,1)
              line$ = Right(line$,Len(line$)-1)
            Wend            
            While FindString(line$,Chr(32)+Chr(32)) >= 1
              line$ = ReplaceString(line$,Chr(32)+Chr(32),Chr(32))
            Wend
            
            RGB$ = line$
            count = CountString(line$,Chr(32))
            R = Val(StringField(RGB$, 1, " "))
            G = Val(StringField(RGB$, 2, " "))
            B = Val(StringField(RGB$, 3, " "))
            Name$ =""          
            For i = 0 To count 
              Name$ + StringField(RGB$, 4+i, " ")            
            Next i
            Name$ = ReplaceString(Name$,Chr(9),"")
            
            ; we draw the color
            x = Mod(j, Round(wcanvas/wcol, #PB_Round_Down)) * (wcol+2)
            y = (j/(wcanvas/wcol)) * (wcol+2)
            
            Box(x, y, wcol, wcol, RGBA(r,g,b, 255))
            j+1
            
            ;  Debug Str(j)+"|"+Str(x)+"/"+Str(y)+"/"+Str(r)+"/"+Str(G)+"/"+Str(B)+"/"
            
          EndIf           
          
        Wend
        
        CloseFile(0)
      EndIf 
      
      
    Else
      ; create some colors, and save it in the file.
    EndIf
    
    ;}
    
    StopDrawing()
  EndIf
  
EndProcedure
Procedure WindowBackgroundEditor()
  
  ; the window background editor
  winW = 800
  WinH = 500
  
  If OpenWindow(#Win_BGeditor, 0, 0, winw, WinH, Lang("Background editor"),#PB_Window_ScreenCentered|#PB_Window_SystemMenu|#PB_Window_Invisible)
    
   ;  HideWindow(#Win_BGeditor,1)

    w1 = WindowWidth(#Win_BGeditor)
    h1 = WindowHeight(#Win_BGeditor)
    Col = RGB(80,80,80)
    col2 = RGB(120,120,120)
    SetWindowColor(#Win_BGeditor,col)
    
    
    ;{ add the gadgets    
    
    ; examine the folder for paper
    paperpath$ = "data\paper\"
    ; Dim tempImg(ArraySize(Thepaper())
    
    
    ; keep the properties of current paper 
    ; (If click on cancel button, I will reassigne those parameters To the current background)
    Define old.sPaper
    old\name$ = OptionsIE\Paper$
    old\scale = paper\scale ; GetGadgetState(#G_paperScale)
    old\alpha = paper\alpha ;GetGadgetState(#G_PaperAlpha)
    old\intensity = paper\intensity; GetGadgetState(#G_PaperIntensity)
    old\color = paper\Color
    
    ;** define some variables for gadgets
    
    ; position of the scrollarea
    y0 = 30
    ; height for the scrollarea and canvas paper & colors
    hh = WinH - 100 - y0
    
    ; width of the canvas for paper image
    wcanvas = winW - 270-40 
    
    ; size of a paper image preview on the canvas
    w2 = wcanvas/4  
    
    ; height for the canvas (paper and colors)
    h2 = (Round((1+ArraySize(Thepaper())) / (wcanvas/w2), #PB_Round_Up))*w2
    
    
    ; the gadgets : 
    
    ;{ buttons and list of background presets
    
    ; the buton to save a background in data\presets\backgrounds
    wb = 22
   ; AddButonImage3(#GADGET_WinBGED_BtnSaveBG, 5, 5, wb, wb, #ico_Save, #PB_Button_Default, 
                   ;lang("Save the background preset (with paper parameters and colors"))
    
    ; the buton to update the list of presets bg, colors, and papers
    ;AddButonImage3(#GADGET_WinBGED_BtnUpdateBG, 5+wb+5, 5, wb, wb, #ico_loop, #PB_Button_Default, lang("Update presets, papers and colors lists"))
    
    
    ; the list of background presets
    wlg = 100
    If ListViewGadget(#GADGET_WinBGED_listBG, 5, y0, wlg, hh)
      
    EndIf
    
    ;}
    
    ;{ paper background
    ; a scrollarea +canvas to see the paper image
    If ScrollAreaGadget(#GADGET_WinBGED_SA_Paper, 10+wlg, y0, winW - 220-wlg, hh, winW - 250,  h2+20)
      SetGadgetColor(#GADGET_WinBGED_SA_Paper, #PB_Gadget_BackColor, col2)      
      ; Debug Round(wcanvas/100, #PB_Round_Down)
      
      If CanvasGadget(#GADGET_WinBGED_Canvas_Paper, 0, 0, wcanvas, h2) 
        If StartDrawing(CanvasOutput(#GADGET_WinBGED_Canvas_Paper))
          Box(0, 0, OutputWidth(), OutputHeight(), col2)
          
          ; then draw the paper image.
          nb = ArraySize(Thepaper())
          For i = 0 To ArraySize(Thepaper())
            tempImg = LoadImage(#PB_Any, paperpath$ + Thepaper(i)\name$)
            ResizeImage(tempImg, w2-5, w2-5)
            x = Mod(i, Round(wcanvas/w2, #PB_Round_Down))*w2
            y = (i/(wcanvas/w2))*w2
            DrawImage(ImageID(tempImg), x, y)
            
            ; draw a box for the name
            DrawingMode(#PB_2DDrawing_AlphaBlend)
            y1 = y+w2-20
            Box(x, y1, w2-5, 20, RGBA(0,0,0, 100))
            
            ; draw the name of the image
            DrawingMode(#PB_2DDrawing_Transparent)
            DrawText(x+10, y1, Thepaper(i)\name$, RGB(255,255,255))
          Next
          
          StopDrawing()
        EndIf
        FreeImage(tempImg)
        
      EndIf
      CloseGadgetList()
    EndIf
    ;}
    
    ;{ color background
    
    ; the buton to save a color for background (use the *.gpl format (palette like gimp/krita/mypaint), the same used for swatchs
    xb = 15+winW - 220
    AddButonImage3(#GADGET_WinBGED_BtnsaveBGColors, xb, 5, wb, wb, #ico_New, #PB_Button_Default, 
                   lang("Create a new color (and save it in the color presets"))

    ; a scrollarea +canvas to see the colors
    If ScrollAreaGadget(#GADGET_WinBGED_SA_colors, xb, y0, 200, hh, 150,  WinH*2)
      
      SetGadgetColor(#GADGET_WinBGED_SA_colors, #PB_Gadget_BackColor, col2)  
      
      
      ; get the number of colors
      wcol = 150/3 - 4
      filecolor$ = GetCurrentDirectory()+ "data\Presets\Background\bgcolors.txt"
      If ReadFile(0, filecolor$)
        j = 0
        While Eof(0) = 0    
          
          line$ = ReadString(0)
          char$ = Mid(line$,1,1)
          
          If char$ <> "#" And line$ <> ""  
            j+1
          EndIf
          
        Wend
        CloseFile(0)
        nbcolors = j-1
        
        hcanvas = j * wcol
        SetGadgetAttribute(#GADGET_WinBGED_SA_colors, #PB_ScrollArea_InnerHeight , (j+1) * wcol)
      Else
        hcanvas = hh
      EndIf
            
      
      ; then create the canvas for color
      If CanvasGadget(#GADGET_WinBGED_Canvas_colors, 0, 0, 150, hcanvas) 
        
        ; create an image to draw the colors, and draw this image on the canvasBGcolor
        Himg = hcanvas
        If hcanvas > 8192
          himg = 8192
        EndIf
        
        tempImgBGcolor = CreateImage(#PB_Any, 150, Himg, 32, #PB_Image_Transparent)
        If tempImgBGcolor 
          UpdateIMageBGColor(tempImgBGcolor, filecolor$, wcol)
        Else
          MessageRequester(lang("Error"), Lang("Unable To create the image For canvas color"), #PB_MessageRequester_Error  )
        EndIf
        
        ;the update the canvas
        UpdateCanvasBGColor(-100,-100,wcol,tempImgBGcolor)
      EndIf
      
      CloseGadgetList()
    EndIf
    
    ;}
    
    ;{ other gagdets (trackbar, button...)
    ; the alpha gadgets (trackbar, name, spin)
    y = y0 + hh+10
    wTB = 150 ; trackbar width
    wSG = 40 ; stringgadget width
    wtg = 55 ; textgadget width
    AddSTringTBGadget(#GADGET_WinBGED_TB_ALphaName,#GADGET_WinBGED_TB_ALpha,#GADGET_WinBGED_TB_ALphaSG, paper\alpha, 
                      lang("Alpha"), lang("Alpha of the background"), 5, y, wTB, wSG, 0, 255, wtg)
    
    ; the scale gadgets (trackbar, name, spin)
    x4 = 5 + wTB + wSG + wtg
    AddSTringTBGadget(#GADGET_WinBGED_TB_scaleName, #GADGET_WinBGED_TB_scale, #GADGET_WinBGED_TB_scaleSG, paper\scale, 
                      lang("Scale"), lang("Scale of the background"), 15+x4, y, wTB, wSG, 1, 50, wtg)
    
    ; the intensity gadgets (trackbar, name, spin)
    AddSTringTBGadget(#GADGET_WinBGED_TB_intensityName, #GADGET_WinBGED_TB_intensity, #GADGET_WinBGED_TB_intensitySG, paper\intensity, 
                      lang("Intensity"), lang("Intensity of the background"), 5, y+30, wTB, wSG, 1, 10, wtg)  
    
    
    ; add two buttons in the bottom
    ButtonGadget(#GADGET_WinBGED_BtnOk,w1-70,h1 -25,60,20,Lang("Ok"))
    ButtonGadget(#GADGET_WinBGED_BtnCancel,w1-140,h1-25,60,20,Lang("Cancel"))
    SetGadgetColor(#GADGET_WinBGED_BtnCancel,#PB_Gadget_BackColor,col2)
    SetGadgetColor(#GADGET_WinBGED_BtnOk, #PB_Gadget_BackColor,col2)
    
    ;}
    
    ;}
    
    HideWindow(#Win_BGeditor,0)
    
     Repeat
      
      Event       = WaitWindowEvent(10)
      EventGadget = EventGadget()
      EventType   = EventType()
      
      Select event
          
        Case #PB_Event_CloseWindow
          quit = 1
          
        Case #PB_Event_Gadget
          
          Select EventGadget
              
            Case #GADGET_WinBGED_Canvas_Paper
              If eventtype = #PB_EventType_LeftButtonDown  
                pos_x = GetGadgetAttribute(#GADGET_WinBGED_Canvas_Paper, #PB_Canvas_MouseX) 
                pos_y = GetGadgetAttribute(#GADGET_WinBGED_Canvas_Paper, #PB_Canvas_MouseY) 
                
                ; to know on which paper image we are
                paperx = pos_x/w2
                papery = pos_y/w2
                
                ; then we change the paper (its only a preview, it will be changed if we confirm by clicking on buton ok
                Idpaper = paperx + papery*4
                ; Debug Str(idpaper)
                ; Debug thepaper(idpaper)\name$
                OptionsIE\Paper$ = thepaper(idpaper)\name$
                PaperUpdate(1)
                ScreenUpdate(0)
              EndIf
              
            Case #GADGET_WinBGED_Canvas_colors
              If eventtype = #PB_EventType_LeftButtonDown  
                pos_x = GetGadgetAttribute(EventGadget, #PB_Canvas_MouseX) 
                pos_y = GetGadgetAttribute(EventGadget, #PB_Canvas_MouseY) 
                
                ; to know on which color image we are
                paperx = pos_x/(wcol+2)
                papery = pos_y/(wcol+2)
                
                ; get the "ID of color
                IdColor = paperx + papery*3
                
                If idcolor <= nbcolors
                  
                  ; get the color
                  If StartDrawing(CanvasOutput(EventGadget))
                    paper\Color = Point(paperx*(wcol+2), paperY*(wcol+2))
                    StopDrawing()
                  EndIf
                  
                  UpdateCanvasBGColor(paperx*(wcol+2), papery*(wcol+2), wcol, tempImgBGcolor)
                  ; Debug Str(idpaper)+"/"+Str(paperx)+"/"+Str(papery) ; +"/"+Str(paper\color)
                  
                  PaperUpdate(1)
                  ScreenUpdate(0)
                
                EndIf
              EndIf
              
            Case #GADGET_WinBGED_TB_ALpha, #GADGET_WinBGED_TB_ALphaSG
              ;{
              paper\alpha = setmin(GetGadgetState(EventGadget), 0)
              SetGadgetState(#GADGET_WinBGED_TB_ALpha, paper\alpha)
              SetGadgetText(#GADGET_WinBGED_TB_ALphaSG, Str(paper\alpha))
              PaperUpdate(1)
              ScreenUpdate(0)
              ;}
              
            Case #GADGET_WinBGED_TB_scale, #GADGET_WinBGED_TB_scaleSG
              ;{
              paper\scale = setmin(GetGadgetState(EventGadget), 1)
              SetGadgetState(#GADGET_WinBGED_TB_ALpha, paper\scale)
              SetGadgetText(#GADGET_WinBGED_TB_ALphaSG, Str(paper\scale))
              PaperUpdate(1)
              ScreenUpdate(0)
              ;}
              
            Case #GADGET_WinBGED_TB_intensity, #GADGET_WinBGED_TB_intensitySG
              ;{
              paper\intensity = setmin(GetGadgetState(EventGadget), 1)
              SetGadgetState(#GADGET_WinBGED_TB_intensity, paper\intensity)
              SetGadgetText(#GADGET_WinBGED_TB_intensitySG, Str(paper\intensity))
              PaperUpdate(1)
              ScreenUpdate(0)
              ;}
              
            Case #GADGET_WinBGED_BtnsaveBGColors
              color = ColorRequester(paper\color)
              If color <> -1
                paper\color = color
                
                ; need to add the new color to the file and to the canvas.
                namecolor$ = InputRequester(lang("Name"), lang("Name for the new color"),"")
                If OpenFile(0, filecolor$, #PB_File_Append)
                  WriteStringN(0, Str(Red(color))+Chr(32)+Str(Green(color))+Chr(32)+Str(Blue(color))+Chr(32)+namecolor$)
                  CloseFile(0)
                EndIf
                
                ; update the canvas-color
                If tempImgBGcolor 
                  UpdateIMageBGColor(tempImgBGcolor, filecolor$, wcol)
                EndIf
                
                UpdateCanvasBGColor(paperx, papery, wcol, tempImgBGcolor)
                
                ; update the paper-background
                PaperUpdate(1)
                ScreenUpdate(0)
              EndIf
              
            Case #GADGET_WinBGED_BtnCancel
              quit =2
              
            Case #GADGET_WinBGED_BtnOk
              quit =1
              
              
          EndSelect
          
          
      EndSelect
      
    Until quit >=1
    
    ; free images
    FreeImage(tempImgBGcolor)
    
    ; close window
    CloseWindow(#Win_BGeditor)
    
    
    ; if click on cancel button, we have to re-update the paper with its previous parameters
    If quit = 2
      OptionsIE\Paper$ = old\name$
      paper\scale = old\scale  
      paper\alpha = old\alpha
      paper\intensity = old\intensity
      paper\Color= old\color
      PaperUpdate(1)
      ScreenUpdate(0)
      
    EndIf
    
    ; and update the gadget for panel options
    SetGadgetState(#G_paperScale, paper\scale)
    SetGadgetState(#G_PaperScaleSG, paper\scale)
    SetGadgetState(#G_PaperAlpha, paper\alpha)
    SetGadgetState(#G_PaperAlphaSG, paper\alpha)
    SetGadgetState(#G_PaperIntensity, paper\intensity)
    SetGadgetState(#G_PaperIntensitySG, paper\intensity)
    
  EndIf
  
  
EndProcedure



;--- HELP
Procedure WindowAbout()
  
  txt$ = lang("Animatoon (version")+Chr(32)+#ProgramVersion+")"+Chr(13)+lang("Developped by Blendman (and many contributers)")+Chr(13)
  
  txtcredit$ ="Fred (Purebasic) & purebasic team (Freak...)"+Chr(13)+lang("B.Vignoli for the base")+Chr(13)+Chr(13)  
  txtcredit$ + "LSI (Image Processing), Venom, G-rom,"+Chr(13)+
               "Kwanjeen, Dobro, Fangbeast, Danilo, "+Chr(13)+
               "Purelust, Netmaestro, Blbltheworm,"+Chr(13)+
               "Rashad, Eddy, Chi, Omy, Idle, Icesoft,"+Chr(13)+
               "Onilink, Kernadec, Falsam, Manalabel,"+Chr(13)+
               "Infratec, ApplePy, Willburt, StarGate..."+Chr(13)+Chr(13)  
  
  txtcredit$ +lang("Icone by :")+Chr(13)+"Sergio Sanchez Lopez, Marco Martin"+Chr(13)+"Oxygen Team, Gnome Project"+Chr(13)+"Creative FreeDom Ltd"+Chr(13)+"David Vignoni"
  txtcredit$ +Chr(13)+"FatCow Web Hosting, Jaanos"
  
  t1$ = lang("Developped By :")+Chr(32)  
  
  
  ;txt_1$ + Chr(13)+ Chr(13)+"Distributed under the LGPL Licence"
  
  If OpenWindow(#Win_About,0,0,400,500,optionsIE\name$,#PB_Window_ScreenCentered|#PB_Window_SystemMenu)
    imgH = 150
    ImgAbout = CreateImage(#PB_Any, 380,imgH);,32, #PB_Image_Transparent)
    ImgAboutCredit = CreateImage(#PB_Any, 380,WindowHeight(#win_About)-80);,32, #PB_Image_Transparent)
    
    LoadImage2(#Img_About,optionsIE\Theme$+"\paint.png" )
    
    ImageGadget(#GADGET_WAboutImage,10,10,380,imgH,ImageID(ImgAbout))
    
    h = WindowHeight(#win_About)-80
    If ScrollAreaGadget(#GADGET_WAboutSA,10,imgH +20,380, WindowHeight(#win_About)-80-imgH,350,h,#PB_ScrollArea_BorderLess)
      ImageGadget(#GADGET_WAboutImageCredit,0,0,380,h,ImageID(ImgAboutCredit))
      CloseGadgetList()
    EndIf
    SetGadgetColor(#GADGET_WAboutSA,#PB_Gadget_BackColor  ,RGB(170,170,170))
    
    
    If StartDrawing(ImageOutput(ImgAbout))     
      Box(0,0,400,WindowHeight(#win_About),RGB(170,170,170))
      DrawAlphaImage(ImageID(#Img_About),5,5)
      
      DrawingMode(#PB_2DDrawing_Transparent)
      yy1 = 30 + ImageHeight(#Img_About)
      xx1 = 10 + ImageWidth(#Img_About)
      yy2 = 30
      DrawingFont(FontID(#fnt_Arial12))
      DrawText(xx1,yy2,t1$,0)
      DrawText(xx1,yy2+40,lang("Version :")+" ",0) 
      
      DrawingFont(FontID(#fnt_Arial12Italic))
      DrawText(xx1+TextWidth(t1$),yy2,"Blendman",0)
      
      DrawingFont(FontID(#fnt_Arial10BoldItalic))
      DrawText(xx1+5+TextWidth(lang("Version :")+" "),yy2+42,#ProgramVersion+" ("+FormatDate("%dd/%mm/%yyyy", #ProgramDate)+")",#Black) 
      StopDrawing()
    EndIf
    
    If StartDrawing(ImageOutput(ImgAboutCredit))
      DrawingFont(FontID(#fnt_Arial10BoldItalic))
      Box(0,0,400,WindowHeight(#win_About),RGB(170,170,170))
      DrawingMode(#PB_2DDrawing_Transparent)
      yy1 = 10
      DrawText(10,yy1, Lang("Thanks to")+" ",0)      
      DrawingFont(FontID(#fnt_Arial12))
      DrawTextEx(10,yy1+20,txtcredit$) 
      StopDrawing()
    EndIf
    
    
    
    SetGadgetState(#GADGET_WAboutImage,ImageID(ImgAbout))
    SetGadgetState(#GADGET_WAboutImageCredit,ImageID(ImgAboutCredit))
    
    ButtonGadget(#GADGET_WAboutBtnOk,170,WindowHeight(#win_About)-50,60,20, lang("Ok"))
  EndIf
  
  
  Repeat
    
    event = WaitWindowEvent(10)
    
    Select event 
        
      Case #PB_Event_Gadget       
        
        Select  EventGadget()
            
          Case #GADGET_WAboutBtnOk           
            QuitAbout = 1
            
        EndSelect
        
      Case #PB_Event_CloseWindow
        
        If GetActiveWindow() = #win_About
          QuitAbout = 1
        EndIf
        
    EndSelect
    
  Until QuitAbout = 1 
  CloseWindow(#Win_About)
  FreeImage2(ImgAbout)
  FreeImage2(ImgAboutCredit)
  FreeImage2(#Img_About)
  
EndProcedure




; IDE Options = PureBasic 5.73 LTS (Windows - x86)
; CursorPosition = 706
; FirstLine = 180
; Folding = 1JAQM--4z3ZY+-
; EnableXP
; EnableUnicode