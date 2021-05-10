﻿
Declare CreateOptionsFile()
Declare AddLogError(error, info$) ; needed for error with initsprite/initkeyboard...

; layers
Declare CreateLayertempo_()
Declare RecreateLayerUtilities()
Declare Layer_FreeAll()
Declare Layer_Add(x=0,y=0,text$="") 
Declare Layer_Clear(i,onlyAlpha=0) 
Declare Layer_UpdateList(u=-1) 
Declare Layer_Update(i)
Declare Layer_convertToBm(i) 
Declare Layer_bm2(i)  
Declare Layer_importImage(update=1)  
Declare Layer_GetBm(id)  
Declare Layer_DrawAll()
Declare Layer_ValidChange(Action,i=-1)  
Declare Layer_Rotate(i,angle)
Declare IE_UpdateLayerUi() 
Declare Layer_updateUi(i)


; brush
Declare UpdateBrushPreview()  
Declare BrushUpdateImage(load=0,color=0) 
Declare OpenPresetBank() 
Declare BrushChangeColor(change=0,color=-1)

; Image
Declare LoadImage2(nb,file$, w=25, h=25)
Declare FreeImage2(img)
Declare CreateImage2(img,w,h,img$,d=24,t=0)

; image processing
Declare ResizeImage2_(image, w, h, mode=#PB_Image_Smooth)
Declare UpdateColorFG()
Declare ScreenUpdate(updateLayer=0)
Declare.l ColorBlending(Couleur1.l, Couleur2.l, Echelle.f) 
Declare.l RotateImageEx2(ImageID, Angle.f, Mode.a=2)
Declare UnPreMultiplyAlpha(image)

; paper
Declare PaperUpdate(load=0)
Declare PaperInit(load=1) 
Declare PaperDraw() 
Declare IE_StatusBarUpdate() ; statusbar 
Declare IE_UpdatePaperList()

; brush color
Declare GetColor(x,y) 
Declare BrushUpdateColor()
Declare BrushResetColor()


; IDE Options = PureBasic 5.73 LTS (Windows - x86)
; CursorPosition = 7
; EnableXP
; EnableUnicode