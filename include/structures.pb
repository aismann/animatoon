﻿

;{ for tablet

Structure LOGCONTEXTA
  lcName.b[40];
  lcOptions.l ;
  lcStatus.l  ;
  lcLocks.l   ;
  lcMsgBase.l ;
  lcDevice.l  ;
  lcPktRate.l ;
  lcPktData.l ;
  lcPktMode.l ;
  lcMoveMask.l;
  lcBtnDnMask.l;
  lcBtnUpMask.l;
  lcInOrgX.l   ;
  lcInOrgY.l   ;
  lcInOrgZ.l   ;
  lcInExtX.l   ;
  lcInExtY.l   ;
  lcInExtZ.l   ;
  lcOutOrgX.l  ;
  lcOutOrgY.l  ;
  lcOutOrgZ.l  ;
  lcOutExtX.l  ;
  lcOutExtY.l  ;
  lcOutExtZ.l  ;
  lcSensX.l    ;
  
  lcSensY.l;
  lcSensZ.l;
  lcSysMode.l;
  lcSysOrgX.l;
  lcSysOrgY.l;
  lcSysExtX.l;
  lcSysExtY.l;
  lcSysSensX.l;
  lcSysSensY.l;
EndStructure

Structure PACKET
  pkButtons.l
  pkX.l
  pkY.l
  pkZ.l
  pkNormalPressure.l
  pkTangentPressure.l
EndStructure

;{ Constantes tablet
#WTI_DEFCTX    = 3
#WTI_DEFSYSCTX = 4


#CTX_NAME          = 1
#CTX_OPTIONS      = 2
#CTX_STATUS        = 3
#CTX_LOCKS        = 4
#CTX_MSGBASE      = 5
#CTX_DEVICE        = 6
#CTX_PKTRATE      = 7
#CTX_PKTDATA      = 8
#CTX_PKTMODE      = 9
#CTX_MOVEMASK     = 10
#CTX_BTNDNMASK   = 11
#CTX_BTNUPMASK   = 12
#CTX_INORGX        = 13
#CTX_INORGY        = 14
#CTX_INORGZ        = 15
#CTX_INEXTX        = 16
#CTX_INEXTY        = 17
#CTX_INEXTZ        = 18
#CTX_OUTORGX      = 19
#CTX_OUTORGY      = 20
#CTX_OUTORGZ      = 21
#CTX_OUTEXTX      = 22
#CTX_OUTEXTY      = 23
#CTX_OUTEXTZ      = 24
#CTX_SENSX        = 25
#CTX_SENSY        = 26
#CTX_SENSZ        = 27
#CTX_SYSMODE      = 28
#CTX_SYSORGX      = 29
#CTX_SYSORGY      = 30
#CTX_SYSEXTX      = 31
#CTX_SYSEXTY      = 32
#CTX_SYSSENSX     = 33
#CTX_SYSSENSY     = 34
#CTX_MAX           = 34

#CXO_MESSAGES  = 4

#WT_DEFBASE    = $7FF0
#WT_PACKET     = #WT_DEFBASE + 0
#WT_INFOCHANGE = #WT_DEFBASE + 6

#PK_BUTTONS           = $0040 ;/* button information */
#PK_X                 = $0080 ;/* x axis */
#PK_Y                 = $0100 ;/* y axis */
#PK_Z                 = $0200 ;/* z axis */
#PK_NORMAL_PRESSURE   = $0400 ;/* normal Or tip pressure */
#PK_TANGENT_PRESSURE  = $0800 ;/* tangential Or barrel pressure */
                              ;}

;}

; utile
Structure Vector2f
  x.f : y.f
EndStructure




; Script
Structure sEvent
  
  Event.i
  id.i
  Array param.i(0)
  
EndStructure 
Structure sScript ; les actions ou script
  
  Array event.sEvent(0)
  
EndStructure
Global Dim script.sScript(0)


; the options
Structure sOptions
  
  ; cursor
  CursorImgId.i
  CursorSpriteId.i
  CursorX.w
  CursorY.w
  CurSorW.w
  CursorH.w
  
  ; theme program
  Lang$
  Theme$
  ThemeColor.i
  ThemeGadCol.i
  ThemeMenuCol.i
  ThemeTBCol.i
  ThemeStaCol.i
  
  
  Debbug.a
  Beta.a ; if we set beta= 1, we have acces to the beta features ?
  SpriteQuality.a
  SaveImageRT.a ; to use the save image in RealTime (screen)
  
  X.w
  Y.W
  
  
  ;{  Options for tools
  LockX.a
  LockY.a
  Proportion.a
  
  ; selection
  Selection.a
  SelectionX.w
  SelectionY.w
  SelectionW.w
  SelectionH.w
  SelectionType.a
  
  ; shapes (box, circle...)
  Shape.a
  ShapeX.w
  ShapeY.w
  ShapeW.w
  ShapeH.w
  ShapeTyp.a
  ShapeParam.a
  ShapeFullLayer.a
  
  ; gradient
  GradientType.a
  GradientBG.i
  GradientFG.i
  ;}
  
  ; general
  Name$
  Version$
  ModeAdvanced.a
  UseRighmouseToPaint.a
  ConfirmAction.a
  
  ; to know if we use the screen as preview or a canvas (screen seems to be faster but some blendmode dosen't work) 
  UseCanvas.a
  UsePaperForRendering.a
  
  
  DoScript.a ; 1 = save, 2 = run
  NbScript.w
  
  ; save
  Autosave.a
  AutosaveTime.i
  AutosaveAtExit.a
  AutosaveFileName$
  ImageHasChanged.a
  ConfirmExit.a
  
  ; Statistics
  NbNewFile.i
  NbMinutes.i
  
  ; Undo
  Maxundo.w
  UndoState.a ; max = 32
  
  ;{ UI 
  Statusbar.a
  
  ; toolbar
  ToolbarH.a
  ToolInfoH.a
  ToolbarFileY.a
  ToolbarFileX.w
  ToolBarFile.a
  ToolbarToolY.a
  ToolbarToolX.w
  ToolbarTool.a
  
  
  ; RB
  RB_Action.a
  RB_Img$
  
  ; swatch
  Swatch$
  SwatchName$
  SwatchColumns.a
  SwatchNbColumns.a
  
  ; pattern
  PatternFile$
  
  ; bordure
  BordureX.w
  BordureY.w
  
  ; TOOLs
  PanelToolsW.w
  PanelToolsH.w
  NbTools.a
  
  ; animation, frame...
  SizeFrameW.w
  AnimBarre.a
  
  ; color  
  selectColor.a ; what is the selector color used ?
  
  
  ; show/hide panel UI
  ShowPanelToolparameters.a
  ShowPanelColors.a
  ShowPanelLayers.a
  ShowPanelswatchs.a
  ShowFullUI.a
  
  ; Tools options
  
  
  ; PANEL LAYERS 
  ; PanelLayerW.w ; Panel Layer W
  ; PanelLayerH.w ; Panel Layer H
  
  ; PANEL colors
  ; PanelColorH.w
  
  
  ; PANEL OPTIOPNS
  AreaBGColor.i
  Paper$ ; the paper of the canvas
  
  ;}
  
  ; Directory and name
  DirPattern$
  DirPreset$
  DirBrush$
  DirBankBrush$
  BrushName$
  
  ; delays  
  Delay.w
  DelayMax.w
  
  
  ; view
  Grid.a
  GridW.w
  GridH.w
  GridColor.i
  
  ; Zoom
  Zoom.w
  
  
  ; layers
  AdjustLayerToImage.a
  LayerTyp.a ; the type of the layer we will create.
  ActionForAllLayers.a
  SelectAlpha.a ; si on a sélectionné l'alpha du calque
  
  ; Filtre
  NoiseDesat.a
  
  ; Document  
  ImageW.w
  ImageH.w
  RealTime.a
  UpdateScreenDelay.w
  
  ; path et file
  PathSave$
  PathOpen$
  OpenDocPatternPos.A
  
EndStructure
Global OptionsIE.sOptions
; define option by default
With OptionsIE
  \Paper$ = "paper0.png"
  \DirBankBrush$ = "data\Presets\Brush\"
  \DirPattern$ = "data\Presets\Pattern\"
  \DirPreset$ = "data\Presets\Bank\bd\"
  \ConfirmExit = 1
  \ToolbarH = 36
EndWith 


; template
Structure sTemplate ;  template de document
  nom$
  format$
  w.w
  h.w
  dpi.w
EndStructure


;{ the document
Structure sDoc
  ; par defaut
  w_def.w
  h_def.w 
  w.w 
  h.w
  name$
  dpi.w
EndStructure
Global Doc.sDoc
With doc
  \w = 1024
  \h = 768
  \w_def = 1024
  \h_def = 768 
  \name$ = "Untitled"
EndWith
;}


; for the presets
Structure sBrushGroup
  ; pour la Treegadget des presets
  group$
EndStructure
Global NewList BrushGroup.sBrushGroup()

Structure sColor1
  Color.i
  Alpha.a
EndStructure
Structure sColor
  R.a
  G.a
  B.a
  A.a
  H.l ; hue
  S.l ; saturation
  L.l ; luminence 
EndStructure

;{ tools (the brush structures)
Structure Brush
  
  Name$
  Version.a ; la version d'animatoon 
  
  Type.a
  Brush.w
  Id.w ; le numero de l'image
  BrushName$ ; the name of the brush (filename$)
  Image$
  Group$
  
  ToolID.a ; the id of the tool, 
  
  
  
  ; alpha
  Alpha.a ; alpha of the BG color
  AlphaOld.a
  alphaMax.a
  AlphaFG.a ; alpha of the Foreground color (use for gradient for example)
  AlphaBlend.w
  AlphaPressure.a ; pression with graphic tablet (wacom)
  AlphaFactorVsTime.w
  AlphaVsTime.w
  AlphaRand.a ; random alpha
  AlphaMin.a ; minimum alpha
  
  AddNoiseRandomOnImagebrush.a
  AddNoiseOnImgMin.w
  AddNoiseOnImgMax.w
  AddNoiseOnImgGrey.w
  AddNoise.a
  
  
  ; size
  Size.w ; sise defined in the UI 
  Sizepressure.a ; to know if we use the pressure or not of the tablet
  SizeRand.w ; use random size
  SizeW.w ; width of size
  sizeH.w ; height of size
  SizeRndFactor.w
  SizeMin.w ; minimum size
  SizeOld.w
  SizeNew.w ; to see if the size has changed, if yes : I resize the image and set sizeOld = sizeNew.
  FinalSize.w ; needed for some action with pressure tablet
  
  
  Transition.a ;  ???
  
  ; dynamics
  Scatter.w ; the scatter of brush
  ScatterMin.w ; the minimum scatter
  
  Rotate.w ; the rotation 
  RotateMin.w ; the minimum rotation
  RandRot.w ; use random rotation
  RotateByAngle.a ; use rotation by angle
                   ; rotation can be cumulated; if we use randrot and rotatebyangle, we will have a random rotation by angle
  
  ; rendu
  Hardness.a ; hardness of the brush (pixel dark are darker
  Softness.a ; softness of the brush (pixel light are lighter)
  Intensity.w
  Smooth.a ; if we use image resized with "smooth" parameter
  Trim.a ; if we "trim" the brush to not have border with alpha =0
  
  ; stroke
  Pas.w ; = the space between two dots
  Trait.a ; If we use a "stroke"
  StrokeTyp.a ; the type of line (rough, gersam...).
  Stroke.a ; ???
  
  
  ; position and center
  X.w
  Y.w
  W.w ; final width of the image after transform (<> sizeW)
  H.w ; final height after tranform // taille final après transformation, ne pas confondre avec SizeW et SizeH
  OldW.w ; 
  OldH.w ; nécessaire pour vérifier si on doit resize le brush
  
  ; Center
  CenterX.w
  CenterY.w
  CenterSpriteX.w
  CenterSpriteY.w
  
  ; colors
  Mix.w
  MixType.a ; Type of mmix // le type de mélange : 1 = vers notre couleur à nouveau, 0= vers la couleur du fond
  MixFade.w ; pour le fade du mix
  Visco.w ; viscosity of the color (the time to change to the new color)
  ViscoCur.w
  Water.a ; if we add some water (erase a few colors on the layer)
  Wash.a ; is the brush washed when mouse UP ? to get the orginial color (color BG) // lave-t-on le brush chaque fois qu'on relève la souris (il reprend la couleur originale)
  MixLayer.a
  
  ; The colors, for mixtype =0 //  pour le mixtype = 0
  Col.sColor
  ColorBG.sColor ; couleur qu'on a pris sur le couleur selector
  NewCol.sColor
  OldCol.sColor
  ColTmp.sColor ; temporary color not used //couleur temporaire pas utilisé ?
  
  ColRnd.sColor ; random color
  
  ; Needed for mixtype= 1 // on garde, car je m'en sers pour le mixtype = 1
  ColorOld.i ; fade to mix // pour le fade vers le mix
  ColorNext.i ; fade to mix // pour le fade vers le mix
  Color.i
  ColorQ.i ; temporary color with mixing // couleur temporaire, avec mixing
  Randcolor.w
  
  ColorFG.i
  
  
  ; other parameters // autres paramètres
  symetry.a ; if we use symetry
  Filter.a
  
  
  ; les actions et type d'outil
  Tool.a ; the type of tool // si brush = pinceau =>
  
  
  ; image used bu brush (paint) // l'image utilisé pour le brush
  BrushNum.w    ; le numéro du brush utilisé dans le dossier "data\brush..."
  BrushNumMax.w ; le nombre max de brush dans le dossier "data\brush..."
  BrushDir$     ; directory des brush
  BrushForm.a   ; circle, square
  
  KeepAlpha.a ; if =1, we paint with drawingmode clipalpha.
  
  ; tools extra parameters
  ; lock the position
  LockX.a 
  LockY.a
  ; use the proportion (box, ellipse...)
  Proportion.a
  
  
  ; clone stamp
  Source.a
  SourceImage.i
  Alignment.a
  
  ; Spray
  Spray.w
  SprayForm.w
  NbSpray.a
  
  ; Particle
  
  ; pickers
  PickerAllLayers.a
  
  ;{ Box, circle
  ShapeOutSize.a
  ShapeOutline.a
  ShapePlain.a
  ShapeType.a     ; normal, round
  RoundX.w
  RoundY.w
  ;}
  
  ; Gradient
  GradientType.a
  
  
EndStructure
Global Dim brush.brush(#Action_Zoom)
Global Action.a = #Action_Brush  

; define by default
For i=0 To #Action_Zoom
  With Brush(i)
    If i <> #Action_Brush
      \size = 1
      \SizeW = 100
      \SizeH = 100
      \alpha = 255
      \AlphaFG = 255
      \Color = RGB(255, 255, 255)
      \ColorBG\R = 255
      \ColorBG\G = 255
      \ColorBG\B = 255
    EndIf
  EndWith
Next 

With Brush(Action) ; define some parameters (no more used ?)
  \Id = 62
  \alpha = 100
  \alphaPressure = 0
  \size = 50
  \SizeW = 100
  \sizeH = 100
  \sizepressure = 1
  \pas = 15
  \transition = 1
  \trait = 1
  \mix = 20
  \Visco = 2
  \RandRot = 360
  \Wash = 1
  \version = 6 ; 6th version ,  c'est la sixième version : GM/PB/teo/agk/pb2-optimised/pb_screen
EndWith

; for the images of brush
Structure sImage
  filename$
  x.w
  y.w
EndStructure
Global Dim BrushImage.sImage(0)
Global NbBrushImage.w = -1
;}

; pattern
Structure sPattern
  name$
  img.i ; the image on the canvas ?
EndStructure
Global Dim Pattern.sPattern(0)



; color, swatch
Structure sSwatch
  col.sColor
  Color.i
  x.w
  y.w
  name$
  img.i
  gadImg.i
EndStructure
Global Dim SwatchColor.sSwatch(0)


Structure sHsv
  h.f
  s.f
  v.f  
EndStructure
Structure Colour
  R.f
  G.f
  B.f  
EndStructure
Global HSV.sHsv



;{ les tiles

; taille des tiles
Global Nx,Ny,Tw,NbTile
Tw = 128
Nx = Round(Doc\W/Tw,#PB_Round_Up)
Ny = Round(Doc\H/Tw,#PB_Round_Up)
NbTile = Nx * Ny

Structure stile
  x.w
  y.w
  image.i
EndStructure


;}


;{ for the layers
Structure sStyle
  
  Name$
  
  DropShadow.a
  DSColor.sColor1
  
  InnerShadow.a
  ISColor.sColor1
  
  OuterGlow.a
  
  InnerGlow.a
  
  Border.a
  BColor.sColor1
  
EndStructure

Structure sLayer
  
  Name$
  id.a ;  c'est l'id unique, défini lorsque le calque est créé, pour la sauvegarde des images (undo)
  X.w : Y.w
  W.w : H.w
  NewW.w : NewH.w
  ImageTemp.i ; nécessaire lors d'opération comme saveimage, merge, etc...
  Image.i
  ImageBM.i ; l'image supplémentaire nécessaire pour certains blendmode. 
            ; Ex multiply a besoin d'une box() entièrement de l'intensité de l'aplha du layer 
            ; (RGBA(layer(id)\alpha,layer(id)\alpha,layer(id)\alpha,255)
  
  ImageAlpha.i ; le canal alpha
  ImageStyle.i ; image pour le style
  
  Sprite.i
  CopySpritetoImg.a ; to know if we need to copy sprite to image
  
  Ordre.w ; l'ordre du calque 0 = tout en bas, 100+ = tout en haut
  
  ImgLayer.i
  ; IG_LayerMenu.i
  
  Typ.a ;normal(bitmap) = 0, texte = 1, background = 2, vecto = 3
  Repeated.a
  W_Repeat.w
  H_Repeat.w
  ;RepeatY.w
  
  ;layer text
  FontName$
  FontSize.w
  FontStyle.w
  FontColor.i
  FontID.i
  Text$
  
  
  ; alphas
  MaskAlpha.a ; a-t-on un calque alpha ?
  Link.a
  
  
  Bm.a
  View.a
  Alpha.a
  
  Group.a
  Locked.a
  LockAlpha.a
  LockMove.a
  LockPaint.a
  
  Style.sStyle
  
  ; autres
  Selected.a ; si on le bouge, affiche un cadre
  ToDelete.a ; on va supprimer le calque
  
  Haschanged.a ; pour vérifier s'il a changé, c'est pour l'autosave par exemple
  
  OkForColorPick.a ; on peut l'utiliser pour prendre la couleur (avec le mode mixtyp en custom
  
  ; transform
  CenterX.w
  CenterY.w
  AngleStartX.w
  AngleStartY.w
  AngleX.w
  AngleY.w
  Angle.w
  
  ; tiles
  Array Tile.stile(0)
  
EndStructure
Global Dim Layer.sLayer(0)
Global LayerNb.w,LayerIdMax.w
;}


;{ background/paper : the papers are in data\papers // il y a le papier qu'on utilise et les images dans le dossier data/paper/
Structure sPaper
  name$
  alpha.a
  scale.w
  intensity.w ; contrast
  brightness.w ; brightness
  imageId.i
  imageIdTexture.i
  Color.i
EndStructure
Global Paper.sPaper
; Define by default
Paper\alpha = 255
Paper\scale = 10
Paper\intensity = 1
Paper\Color = RGB(255, 255, 255)

; for the paper editor and paper list
Global Dim Thepaper.sPaper(0) 

;}



;{ stroke and dots
Structure sDot
  ; ce sont les variables finales, après tous les calculs de chaque paramètres
  X.w ; x+ scatter
  Y.w
  W.w
  H.w
  Alpha.a
  ; scatter.w
  Rot.w ; rot+randrot
  Size.w
  SizeW.a ; taille en W et H
  SizeH.a
  Colo.a
  Sprite.i
  
EndStructure


Structure sStroke
  
  ; quand on dessine, on pose des "points". 
  ; Moi, je calcule un trait entre ces points posées par la souris (dans la boucle avec b, dist, etc...
  ; au lieu d'ensuite dessiner, je vais calculer une seule fois tous les points du trait et stocké ce calcul (donc le trait opbtenu)
  ; dans un tableau de stroke.
  ; je m'en servirai pour le undo/redo
  ; je garderai uniquement les calculs finaux, comme si j'allais afficher chaque brush (sur le screen puis l'image
  ; j'ai donc un tableau de trait (stroke) et chaque trait a un tableau de points (par rapport au brush\pas)
  ; à chaque undo, je reviendrai de 1 en arrière dans le tableau et si je suis à l'élément 0 , je reviens au max puis, max-1, etc...
  Layer.a ; ?
  Array dot.sDot(0)
  
EndStructure
Global Dim Stroke.sStroke(0)
Global StrokeId.a

;}




; plugins
Structure sPlugins
  
  menuId.w
  lib.i
  name$
  
EndStructure
Global NewList Ani_Plugins.sPlugins()







; IDE Options = PureBasic 5.73 LTS (Windows - x86)
; CursorPosition = 709
; FirstLine = 56
; Folding = AAAACUh
; Markers = 389
; EnableXP
; EnableUnicode