﻿; patterns for animatoon




; update
Procedure UpdatePatternsCanvasPanel(x=-100,y=-100)
  
  Shared patternCaseW
  
  ; update the canvas with image of patterns
  
  w = ImageWidth(#image_patternscanvas)
  h = ImageHeight(#image_patternscanvas)
  
  
  ;wcanvas = GadgetWidth(#G_PatternCanvas)
  wcol = patternCaseW ; (wcanvas/2) -16
  
  ;x*wcol
  ;y*wcol
  
  ; resize the gadgets (scrollarea for panel patterns
  ; SetGadgetAttribute(#G_SA_Pattern,  #PB_ScrollArea_InnerWidth ,w)
  If h > GetGadgetAttribute(#G_SA_Pattern,  #PB_ScrollArea_InnerHeight)
    SetGadgetAttribute(#G_SA_Pattern,  #PB_ScrollArea_InnerHeight ,h)
  EndIf
  ; update the canvas
  If StartDrawing(CanvasOutput(#G_PatternCanvas))
    
    Box(0, 0, OutputWidth(), OutputHeight(), RGB(255,255,255))
    
    DrawingMode(#PB_2DDrawing_AlphaBlend)
    DrawAlphaImage(ImageID(#image_patternscanvas),0,0)
    
    ; then draw the selected color
    DrawingMode(#PB_2DDrawing_Outlined )
    Box(x,y,wcol,wcol, RGB(255,0,0))
    Box(x+1,y+1,wcol-2,wcol-2, RGB(0,0,0))
    
    StopDrawing()
  EndIf
  
EndProcedure

Procedure UpdatePatternsImage()
  
  Shared patternCaseW
  
  ; delete images of pattern
  For i =0 To ArraySize(Pattern())
    freeimage2(Pattern(i)\img)
  Next
  
  ; chack the images of patterns
  size = 0
  dir$ = GetCurrentDirectory() + OptionsIE\DirPattern$
  If ExamineDirectory(0, dir$, "*.*")  
    
    While NextDirectoryEntry(0)
      
      If DirectoryEntryType(0) = #PB_DirectoryEntry_File
        
        ; add the name of the image to the array pattern()
        Name$ = DirectoryEntryName(0)
        
        ; redim the array pattern with new size
        ReDim Pattern(size)
        Pattern(size)\name$ = Name$
        size = ArraySize(Pattern())+1
        
      EndIf
      
    Wend
    FinishDirectory(0)
    
  EndIf
  
  
  ; change the size of canvas if needed
  wcanvas = GadgetWidth(#G_PatternCanvas)
  w2 = (wcanvas/2) -16
  patternCaseW = w2
  h2 = (Round((2+ArraySize(pattern())) / (wcanvas/w2), #PB_Round_Up))*w2
  
  ResizeGadget(#G_PatternCanvas, #PB_Ignore, #PB_Ignore, #PB_Ignore, h2)
  
  
  ; to update the patterns on the panel patterns
  freeimage2(#image_patternscanvas)
  
  If CreateImage(#image_patternscanvas, wcanvas, h2 , 32, #PB_Image_Transparent) = 0            
    
    MessageRequester(Lang("Error"), Lang("Unable to create the image for the patterns (canvas)"))
    ProcedureReturn 0
  EndIf
  

  
  ; update the image of patterns
  If StartDrawing(ImageOutput(#image_patternscanvas))
    
    ; clear the image
    DrawingMode(#PB_2DDrawing_AllChannels)
    Box(0, 0, OutputWidth(), OutputHeight(), RGBA(0,0,0,0))
    
    ; draw the images of patterns
    
    
    For i = 0 To ArraySize(Pattern())
      
      ; tempimg = Pattern(i)\img
      tempImg = LoadImage(#PB_Any, dir$ + Pattern(i)\name$)
      
      If tempImg <> 0
        
        ;Debug "error "+dir$+Pattern(i)\name$
        
        ResizeImage(tempImg, w2, w2)
        x = Mod(i, Round(wcanvas/w2, #PB_Round_Down))*w2
        y = (i/(wcanvas/w2))*w2
        DrawImage(ImageID(tempImg), x, y)
        
        FreeImage(tempImg)
        
      EndIf
      
    Next i
    
    StopDrawing()
  EndIf
  
  ; update the canvas of patterns
  UpdatePatternsCanvasPanel()
  
EndProcedure

; create the patterns, on the panel pattern
Procedure CreatePatterns()
  
  ; create the patterns on the panel patterns
  
  
  ;   ; create a image to draw the pattern on it
  ;   If CreateImage(#image_patternscanvas, 100,200) = 0            
  ;     
  ;     MessageRequester(Lang("Error"), Lang("Unable to create the image for the patterns (canvas)"))
  ;     
  ;     ;   Else
  ;     ;     
  ;     ;     If StartDrawing(ImageOutput(#image_patternscanvas))
  ;     ;       Box(0, 0, OutputWidth(), OutputHeight(), RGB(255, 255, 255))
  ;     ;       StopDrawing()
  ;     ;     EndIf
  ;     
  ;   EndIf
  
  
  ; then update
  UpdatePatternsImage()
  
EndProcedure

Procedure SetStampPatternImage(x=-100, y=-100, panel = 1)
  
  Shared patternCaseW
  
  If panel = 0
    ; if we get pattern form a layer
    
    
  ElseIf panel =1
    
    ; we get pattern form a pattern image, from the pattern panel.
    
    ; to know on which pattern we are
    w2 = patternCaseW ;(wcanvas/2) -16
    patternx = x/w2
    patterny = y/w2
    
    ; get the "ID of pattern
    Idpattern = patternx + patterny*2
    
    patternx * w2
    patterny * w2
    
    ; update the panel canvas for pattern
    If Idpattern <= ArraySize(pattern())
      
      ; update the pattern canvas (on panel)
      UpdatePatternsCanvasPanel(patternx, patterny)
      
      ; update the image of the pattern for the stamp tool
      FreeImage2(#image_patternForstamp)
      
      
      ; create the new pattern image
      theimage$ = GetCurrentDirectory()+OptionsIE\DirPattern$+pattern(Idpattern)\name$
      
      If CreateImage(#image_patternForstamp, Doc\w, doc\h, 32, #PB_Image_Transparent) = 0
        AddLogError(1, "Unable to create the pattern stamp image : "+ theimage$)
        MessageRequester(lang("Error"), Lang("unable to create the pattern image "+pattern(Idpattern)\name$))
      Else
        
        tempimg = LoadImage(#PB_Any,  theimage$)
        
        If tempimg <> 0
          ; draw on the image pattern
          If StartDrawing(ImageOutput(#image_patternForstamp))
            w =  OutputWidth()
            h =  OutputHeight()
            
            For i =0 To doc\w /w
              
              For j = 0 To doc\h /h
                DrawAlphaImage(ImageID(tempimg), i * w, j * h)
              Next j
              
            Next i
            
            StopDrawing()
          EndIf
          
          FreeImage(tempimg)
          
        EndIf
        
      EndIf
      
      
    EndIf
        
  EndIf
  
  
  
EndProcedure


; IDE Options = PureBasic 5.73 LTS (Windows - x86)
; CursorPosition = 90
; FirstLine = 59
; Folding = NK--
; EnableXP