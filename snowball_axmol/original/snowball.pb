;*****************************************************************************
;*
;* XMas 2014 PurePunch contest
;* PureBasic 5.31
;* 64 lines of 80 chars of code + 78 (only!) lines of data
;*
;* Name     : Snowball
;* Author   : Majikeyric (http://majikeyric.free.fr)
;* Date     : 08/12/2014
;*
;* Are you ready for a snow battle with your friends ?
;* 
;* Your are the blue player,
;* When you shoot another player you gain 1 point.
;* When your are shot you lost 1 point.
;* The first with 10 points wins the game !
;* 
;* Controls:
;* 
;* Cursor Left and Cursor Right to move
;*
;* Cursor Down to pick up snow + Space to throw it
;*
;* Credits  : original C64 graphics by Ashley Routledge (from Snowball sunday)
;*
;*****************************************************************************
EnableExplicit

#P=2920					; taille des data packées
#U=13431				; taille des data non packées
#SS=8135				; taille du fichier .mid
#L=640					; largeur de l'écran
#H=480					; hauteur de l'écran
#NS=13					; nombre de sprites par joueur (dans une seule direction)
#NbFlocons=200			; Nombre de flocons de neige sur la totalité du décor
#Y=380					; position vertivale des persos
#Tempo=90				; Nombre de balayages écrans pendant lesquels un perso "se replie" après avoir tiré
#DistanceTir=194		; distance à laquelle les perso se mettent de leur cible pour tirer

Macro M:Macro:EndMacro
Macro E:End:EndMacro
Macro _:Data:EndMacro
Macro SD(a):st#a#drawing:EndMacro

Macro MakeSprite2(v)

	j=CreateSprite(#PB_Any,24,21,#PB_Sprite_PixelCollision)
	sprites(i+#NS*(v))=j
	SD(art)(SpriteOutput(j))
	For k=0 To 20
		For l=0 To 23
			Plot(l,k,s2\b[k*24+l])
		Next
	Next
	SD(op)()
	ZoomSprite(j,48,42)

EndMacro

Macro MakeSprite(c,v)

	s\p[2]=c
	ConvertitSpriteC64EnSprite256Coul(@s,@s2)
	MakeSprite2(v)
	InverseSprite256Coul(@s2)
	MakeSprite2(v+1)

EndMacro

Macro Init
ClearList(Acteur())
For i=0 To 2
	AddElement(Acteur())
	Acteur()\Direction=0
	Acteur()\Numero=i
	Acteur()\Y=#Y
	Acteur()\X=Random(320) & $fffe
	Acteur()\BouleDeNeige=0
	Acteur()\Phase=#ChoixCible
	Acteur()\Cible=0
	Acteur()\Etat=0
	Acteur()\CompteurAnim=1
	Acteur()\IndiceAnim=1
	Score(i)=0
	op(i)=Acteur()
Next
ClearList(BouleDeNeige())
d=""
*op1=op(0)
EndMacro

Enumeration PhaseDesPersonnages
	#ChoixCible
	#RamassageNeige
	#MiseaDistance
	#Tir
	#Repli
EndEnumeration

Enumeration EtatDesPersonnages
	#Immobile
	#Marche
	#RamasseBouleDeNeige
	#LanceBouleDeNeige
	#ChuteAvant
	#ChuteArriere
EndEnumeration

Enumeration Direction
	#DirDroite
	#DirGauche
EndEnumeration

Structure _Map
	m.a[25*81]
EndStructure

Structure _Anim
	a.a[6*16]
EndStructure

Structure car64
	b.a[8]
	p.l[4]
EndStructure

Structure car256
	b.l[8*8]
EndStructure

Structure sp64
	b.a[64]
	p.l[4]
EndStructure

Structure sp256
	b.l[24*21]
EndStructure

Structure Flocon
	X.i
	Y.f
	V.f
EndStructure

Structure Acteur
	Numero.i
	Direction.i
	X.i
	Y.i
	Etat.i
	Phase.i
	*Cible.Acteur
	Tempo.i
	BouleDeNeige.i
	CompteurAnim.i
	IndiceAnim.i
	Sprite.i
EndStructure

Structure BouleDeNeige
	Tireur.i
	X.f
	Y.f
	VX.f
	VY.f
EndStructure

Define *s,*d,*p
Define.i i, j, k, l, x, ox, dx, SpriteFlocon
Define.Acteur *op1,*op2
Define.s d, o
Define s.sp64, s2.sp256, c.Car64, c2.Car256
Define.i ActeurDroite, ActeurGauche, ActeurBas, ActeurTir
Define.f Gravite

NewList Flocon.Flocon()
NewList Acteur.Acteur()
NewList BouleDeNeige.BouleDeNeige()

Dim op.i(3)
Dim TileSet.i(256)
Dim p.l(16)
Dim Score.i(3)

Global Dim sprites.i(#NS*2*3+1)
Global *m._Map,*a._Anim

Gravite=0.2

Procedure Anime(*acteur.Acteur, anim.i)
	
	Protected.i f
	
	f=0
	*acteur\CompteurAnim-1
	
	If *acteur\CompteurAnim=0
		*acteur\CompteurAnim=*a\a[anim<<4]
		If *a\a[anim<<4+*acteur\IndiceAnim]=0
			f=1
			*acteur\IndiceAnim=1
		EndIf
		*acteur\Sprite=sprites(*a\a[anim<<4+*acteur\IndiceAnim] + *acteur\Numero*#NS*2 + *acteur\Direction*#NS)
		*acteur\IndiceAnim+1		
		
	EndIf
	ProcedureReturn f
	
EndProcedure

Procedure ChangeEtat(*acteur.Acteur,s)
	*acteur\Etat=s
	*acteur\CompteurAnim=1
	*acteur\IndiceAnim=1
EndProcedure

Procedure ConvertitSpriteC64EnSprite256Coul(*s.sp64,*d.sp256)
	
	Protected.i i,j,k,l,z,d
	Protected.l c
	
	l=0
	For k=0 To 20
		For i=0 To 2
			d=6
			For j=0 To 3
				c=*s\p[*s\b[l]>>d&3]
				z=k*24+i*8+j*2
				*d\b[z]=c
				*d\b[z+1]=c
				d-2
			Next
			l+1
		Next
	Next
	
EndProcedure

Procedure ConvertitCaractereC64EnCaractere256Coul(*s.car64,*d.car256)
	
	Protected.i j,k,l,z,d
	Protected.l c
	
	l=0
	For k=0 To 7
		d=6
		For j=0 To 3
			c=*s\p[*s\b[l]>>d&3]
			z=k*8+j*2
			*d\b[z]=c
			*d\b[z+1]=c
			d-2
		Next
		l+1
	Next
EndProcedure

Procedure InverseSprite256Coul(*s.sp256)
	
	Protected.i i,j,z,z2
	Protected.l p1
	
	For i=0 To 20
		For j=0 To 11
			z=i*24+j
			z2=i*24+(23-j)
			p1=*s\b[z]
			*s\b[z]=*s\b[z2]
			*s\b[z2]=p1
		Next
	Next
	
EndProcedure

UseLZMAPacker()

*s=AllocateMemory(#P)
*d=AllocateMemory(#U)
Restore a
*p=*s
i=#P
Read.s d
While i
	o=Left(d,2)
	d=Mid(d,3)
	If o="" And i;>0 ;OPTI
		Read.s d
	ElseIf o;<>"" ;OPTI
		PokeA(*p,Val("$"+o))
		*p+1
		i-1
	EndIf
Wend

If UncompressMemory(*s,#P,*d,#U,#PB_PackerPlugin_LZMA)=-1
	Debug "erreur decompression !"
EndIf

FreeMemory(*s)

InitKeyboard()
InitSprite()
InitMovie()

OpenWindow(0,0,0,#L,#H,"Snowball by Majikeyric",#PB_Window_ScreenCentered)
OpenWindowedScreen(WindowID(0),0,0,#L,#H)

TransparentSpriteColor(#PB_Default,$ff00ff) ; Magenta

; traitement des données

*s=*d
CopyMemory(*s,@p(0),64)
*s+64

s\p[0]=$ff00ff
s\p[1]=0
s\p[3]=p(10)
i=1
Repeat
	CopyMemory(*s,@s\b[0],64)
	MakeSprite(p(6),0)
	MakeSprite(p(2),2)
	MakeSprite(p(12),4)
	*s+64
	i+1
Until i=#NS+1

c\p[0]=$ff00ff
c\p[1]=p(1)
c\p[2]=p(15)

*p=*s
i=0
Repeat
	CopyMemory(*s,@c\b[0],8)
	c\p[3]=p(PeekA(*p+2048+i)-8)
	ConvertitCaractereC64EnCaractere256Coul(@c,@c2)
	j=CreateSprite(#PB_Any,8,8)
	TileSet(i)=j
	SD(art)(SpriteOutput(j))
	For k=0 To 7
		For l=0 To 7
			Plot(l,k,c2\b[k*8+l])
		Next
	Next
	SD(op)()
	ZoomSprite(j,16,16)
	*s+8
	i+1
Until i=256

*s+256
*m=*s

*s+2000
i=CreateFile(#PB_Any,"1.mid")
WriteData(i,*s,#SS)
CloseFile(i)
*s+#SS
*a=*s

For i=0 To #NbFlocons-1
	AddElement(Flocon())
	Flocon()\Y=Random(479)
	Flocon()\X=Random(1279)
	Flocon()\V=Random(2,1)+Random(9,1)/10
Next

SpriteFlocon=CreateSprite(#PB_Any,2,2)
StartDrawing(SpriteOutput(SpriteFlocon))
Box(0,0,2,2,#White)
StopDrawing()

LoadMovie(0,"1.mid")

Init

Repeat
	
	Repeat
	Until WindowEvent()=0
	
	If MovieStatus(0)=0
		PlayMovie(0,WindowID(0))
	EndIf
	
	ExamineKeyboard()
	
	If d=""
		
		ForEach Acteur()
			
			If Acteur()\Numero
				
				ActeurTir=0
				ActeurGauche=0
				ActeurDroite=0
				ActeurBas=0
				*op2=op(Acteur()\Numero!3)
				
				Select Acteur()\Phase
						
					Case #ChoixCible
						If Random(1)=1
							Acteur()\Cible=*op1
						Else
							Acteur()\Cible=*op2
						EndIf
						
						Acteur()\Phase=#RamassageNeige
						
					Case #RamassageNeige
						
						If Acteur()\BouleDeNeige=0
							ActeurBas=1
						Else
							Acteur()\Phase=#MiseaDistance
						EndIf
						
					Case #MiseaDistance
						
						i=Acteur()\X - Acteur()\Cible\X
						
						If Abs(i)>#DistanceTir
							; on se rapproche
							If i<0
								ActeurDroite=1
							Else
								ActeurGauche=1
							EndIf
						ElseIf Abs(i)<#DistanceTir
							; on s'éloigne
							If i<0
								ActeurGauche=1
							Else
								ActeurDroite=1
							EndIf
						Else
							If i<0
								Acteur()\Direction=#DirDroite
							Else
								Acteur()\Direction=#DirGauche
							EndIf
							Acteur()\Phase=#Tir
						EndIf
						
					Case #Tir
						If Acteur()\BouleDeNeige=1
							ActeurTir=1
						Else
							Acteur()\Phase=#Repli
							Acteur()\Tempo=#Tempo
							Acteur()\Direction!1
						EndIf
						
					Case #Repli
						Acteur()\Tempo-1
						If Acteur()\Direction=#DirGauche
							ActeurGauche=1
						Else
							ActeurDroite=1
						EndIf
						If Acteur()\Tempo=0
							Acteur()\Phase=#ChoixCible
						EndIf
						
				EndSelect
				
			Else
				ActeurTir=KeyboardPushed(#PB_Key_Space)
				ActeurGauche=KeyboardPushed(#PB_Key_Left)
				ActeurDroite=KeyboardPushed(#PB_Key_Right)
				ActeurBas=KeyboardPushed(#PB_Key_Down)
			EndIf
			
			Select Acteur()\Etat
					
				Case #ChuteAvant
					If Anime(Acteur(),#ChuteAvant)
						ChangeEtat(Acteur(),#Immobile)
						;Anime(Acteur(),Acteur()\Etat);
					EndIf
				Case #ChuteArriere
					If Anime(Acteur(),#ChuteArriere)
						ChangeEtat(Acteur(),#Immobile)
						;Anime(Acteur(),Acteur()\Etat);
					EndIf
				Case #RamasseBouleDeNeige
					If Anime(Acteur(),#RamasseBouleDeNeige)
						Acteur()\BouleDeNeige=1
						ChangeEtat(Acteur(),#Immobile)
						;Anime(Acteur(),Acteur()\Etat);
					EndIf
				Case #LanceBouleDeNeige
					If Acteur()\IndiceAnim=2 And Acteur()\CompteurAnim=1
						Acteur()\BouleDeNeige=0
						AddElement(BouleDeNeige())
						BouleDeNeige()\Tireur=Acteur()\Numero
						BouleDeNeige()\Y=Acteur()\Y+10
						BouleDeNeige()\VY=-2.5
						If Acteur()\Direction=#DirDroite
							BouleDeNeige()\X=Acteur()\X+45
							BouleDeNeige()\VX=7
						Else
							BouleDeNeige()\X=Acteur()\X-5
							BouleDeNeige()\VX=-7
						EndIf
					EndIf
					If Anime(Acteur(),#LanceBouleDeNeige)
						ChangeEtat(Acteur(),#Immobile)
						;Anime(Acteur(),Acteur()\Etat);
					EndIf
				Default
					If ActeurBas
						ChangeEtat(Acteur(),#RamasseBouleDeNeige)
					ElseIf ActeurTir And Acteur()\BouleDeNeige=1
						ChangeEtat(Acteur(),#LanceBouleDeNeige)
					ElseIf ActeurDroite
						Acteur()\Direction=#DirDroite
						If Acteur()\Etat<>#Marche
							ChangeEtat(Acteur(),#Marche)
						EndIf
						Acteur()\X+2
					ElseIf ActeurGauche
						Acteur()\Direction=#DirGauche
						If Acteur()\Etat<>#Marche
							ChangeEtat(Acteur(),#Marche)
						EndIf
						Acteur()\X-2
					Else
						ChangeEtat(Acteur(),#Immobile)
					EndIf
					
					Anime(Acteur(),Acteur()\Etat)
					
			EndSelect
		Next
	EndIf
	
	ClearScreen(p(14))
	
	StartDrawing(ScreenOutput())
	Box(0,400,640,80,#White)
	StopDrawing()
	
	; Centrage horizontal de la fenêtre sur le joueur
	
	ox=*op1\X+24-320
	
	If ox<0
		ox=0
	ElseIf ox>640
		ox=640
	EndIf
	
	; Affichage du décor
	
	x=ox/16:dx=ox%16
	
	For i=0 To 24
		For j=0 To 40
			l=*m\m[i*80+x+j]
			If l
				DisplayTransparentSprite(TileSet(l),j*16-dx,i*16)
			EndIf
		Next
	Next
	
	If d=""
		ForEach Acteur()
			DisplayTransparentSprite(Acteur()\Sprite,Acteur()\X-ox,Acteur()\Y)
		Next
		
		ForEach BouleDeNeige()
			BouleDeNeige()\X+BouleDeNeige()\VX
			BouleDeNeige()\Y+BouleDeNeige()\VY
			BouleDeNeige()\VY+Gravite
			
			If BouleDeNeige()\Y>420
				DeleteElement(BouleDeNeige())
			Else
				DisplayTransparentSprite(sprites(#NS+4*#NS),BouleDeNeige()\X-ox,BouleDeNeige()\Y)
				If BouleDeNeige()\Y<410
					ForEach Acteur()
						If SpritePixelCollision(sprites(#NS+4*#NS),BouleDeNeige()\X,BouleDeNeige()\Y,Acteur()\Sprite,Acteur()\X,Acteur()\Y) And
						   Acteur()\Etat<>#ChuteArriere And
						   Acteur()\Etat<>#ChuteAvant
							Score(Acteur()\Numero)-1
							Score(BouleDeNeige()\Tireur)+1
							If (Acteur()\Direction=#DirDroite And BouleDeNeige()\VX<0) Or
							   (Acteur()\Direction=#DirGauche And BouleDeNeige()\VX>0)
								ChangeEtat(Acteur(),#ChuteArriere)
							Else
								ChangeEtat(Acteur(),#ChuteAvant)
							EndIf
							DeleteElement(BouleDeNeige())
							Break
						EndIf
					Next
				EndIf
			EndIf
		Next
	EndIf
	
	ForEach Flocon()
		If Flocon()\X>ox And Flocon()\X<ox+640
			DisplaySprite(SpriteFlocon,Flocon()\X-ox,Flocon()\y)
			Flocon()\Y+Flocon()\V
			If Flocon()\Y>420
				Flocon()\Y=0
			EndIf
		EndIf
	Next
	
	For k=0 To 2
		DisplayTransparentSprite(sprites(1+#NS*2*k),580,k*42)
		StartDrawing(ScreenOutput())
		DrawText(620,k*42+18,Str(Score(k)))
		StopDrawing()
	Next
	
	If Score(1)=10 Or Score(2)=10
		d="loose"
	ElseIf Score(0)=10
		d="win"
	EndIf
	
	If d
		SD(art)(ScreenOutput())
		DrawText(280,220,"You "+d+" !")
		SD(op)()
		If KeyboardPushed(#PB_Key_Space)
			Init
		EndIf
	EndIf
	
	FlipBuffers()
	
Until KeyboardPushed(#PB_Key_Escape)

DeleteFile("1.mid")
FreeMemory(*d)

_#Section:a: :_.s"5D00000001000068BE5100372D947F456664499A252B19095F5D99422F6A",;1
"564DC1180EA39182B5994FE60F566CF97F5423C7315C8BA305145FEB5DA1C6C01C703F870154",;2
"5A77D3B6DBF5C5FEFE4E14C70AB1D5D2489689657E08BF864BBB41842CF7F973BBC4F8098C67",;3
"49BD036D048D1A5FC2B841323830A0BC2775687DA7952197BF69D340FF0F259CCEC7990849B0",;4
"500F5F984D9A8C9B03F481FC7A896CAA7A38E90602EC3E281D402AB8EB1B8DBA44A9FB7EA3E5",;5
"06D3BFBA7D5FCE4EDC06FA06A555187BB8DF10927FB95E3F7F9AFA087A4A35EAFCB8F7FD0A1D",;6
"A1D71E2E14503F579213B55F925AF4AF1A234F2B978985F4F6C74271738975431FAE00A5366D",;7
"A44EE4EC63482A7C87CE03BD683134E153DB4899979448C57952535A178EB3CA0085247C4264",;8
"92877A4BE8E3BA922344E394B6096C8C0D4E6E32AB089376356ED3489881BAD37EF73378F147",;9
"EE154E2D6CC98BEEE0E568653AFAACB3DBB696E71348AA1E88103F9A741917B0E612F31652AE",;10
"F0A3D14505889F9A24575B15E743542F62CE1F849DBECEB046103212D5260ECB8BC0FA08E336",;11
"AFEBBEE6635B805A9A2CE0C6D6640323777ECF6DB16CC0CB3F536B8A9B49B668B178548B0BB4",;12
"D91FF2A8DDD30742A10522C11F4B2D3CED244D413947FD86C3911AFF559153CA85E74ACDBE51",;13
"5B926E204AEA64238446054CEF2923936CC8B2016327D04A89EEA1DE81ED458393D207DFBDA2",;14
"731E6AAD21014D73E722CAF6B1D57F7245F03C56E6EADED6A7FEBEA69DAB93E134BCFDEF692C",;15
"18859ED3D7812C289F649A32BCF85EE3A01C5319352F9AF4E6C1ECE4354423C0AABFEB08359E",;16
"59871EE521B70B4D4F32155E353452721F639F01CF1288ADE9C1AE8438DB3330276F9FD13B65",;17
"6C31C30B471D9D86FE26B0D9A5D7F7C51C89ABE99E521EAAA3B16AFF2263449524D3C050A4B9",;18
"4A03E6CB9433C85EDAFD28FAC385CDF3D1A31F34098823D7402335AEDBAFE73F160CCEA68900",;19
"F8D715E32B6C1ADBA65450C1206C6FC16E4C81D5642D0C4FE60220B4817BC6EF885D3E87FFDE",;20
"66F99998EB3D9D4AF2335FD35A4CB7B12E71393878FEC6BE9153C2DAB1FEE3E109E34ACFC9D8",;21
"83CA42F4D576A9AA4AEF6ABE67EB8D0071F8E9C29FC90AEB3663231716B663BCD1CB95E07743",;22
"DC4ECAF07F2E73FEA256C98885CB576306A9F2B40496046F2BFDFCDDA99FA3A786D7D892FA3B",;23
"42C533D0880AD365EFD09A615E2D7D92A1F9DAE6CBDCC6E11385B900913853095328C85B9CBD",;24
"C2599CA6C73E8DCAE9D732BDB73FD2F6D48581F66CCA12040B691802571808A5A75421CF2061",;25
"745484BCEE1D41C55B3B9A872A88027C6BB93088C04795344F0C4DF58D9D4C23BFB56BF010D5",;26
"10963B6F18FAAA57CA90A46B205295B786A8A5B3E26EB8A80C5284BB0BB4151B0BB3194AB8EB",;27
"9ED87A5C5475C086A5FF8ED206BB1575B02195BCECEFEDA14A3CEF9BCF38B7A6446A869BC2D8",;28
"2CF149B1F882BDFEB69E6BCD6456E4FF21326F4C12C9A6576EDE5BDB1581F38061F13EE8B1FF",;29
"23E63571569A3725CF2F0384150D32A4385612C4019E83E9DC30608617AF1421BA5CC7554036",;30
"50BFF9680F7014AC4BD6AA7639F48DFE10488D4D839EB77C4C9B01877F977E60D726D8EF5A3A",;31
"09AB4F5F5F593EC5AFBC87C13469D92CB2ED157F07A75927446D7708E2C9E7A29482D0C07257",;32
"8F6777A6D4DB32269AFD9CEA2265C70792B07DEA52E4987BB3F3DFEAB047EA467529715A5436",;33
"81178061B587E49F3D807AA915C06CC83109B8DD6D3BBDF0AE44C9D27AF64EA3D08DF351114D",;34
"2BAD9D50CEC0039AA25F15E339C412A3D482A22DEC48687D3929DD1CB863B74F26CCC40D3124",;35
"8C3AE0F48FAF004062368910E211D3C4B6C76FAF4B93FA8D61EC9E95020D87795C2DDCB8AC0E",;36
"331444E710FB1CCF74A1F91020B59B24623F0ECBB8E4FA42051292E3FFF8239C99B2C358A92A",;37
"178219699D0538620066539E612FD69940F84784C32970F97789F5FEB513CFA65402E0350044",;38
"6DB862EFD926654EF519E75434823CE9316292DBA3BEB96F43E1916CBAFEDD5176D548A4A61E",;39
"03E9FB89D1EB5BD3CC79054317E06190031BB54F62CC77A6971F6D241225C64A367E5504B8A4",;40
"AA78682246F0375CBDC44A69285F3B9FEBCEA02C65757D914E891125582E90DECA8CE96C84BA",;41
"C600C03AA33175A1FF42F350FBE2D867FECDF36B48B3BAEB5F31D3D0660E7CC2428602BE14A5",;42
"812EAA62B09A465EF1C71E28AD7DD5C75D4EF8C67B18A7EC3DB13742C1E3E0CB9DB86FF44F7F",;43
"59F0593FB8382359DEF70CA94C0B917B5CF1E76F84E1F07D096C132941380D5836F699B8F219",;44
"0A24008BA30A8B46D8AD1B0A3F027FB335323166005592E05B8E77FA4190EE75513C618A9875",;45
"A2DC66652AB61F65ACD25587C6E54047AE9644E302FA7565B36EADAF8079A65DAF6A509DCE4E",;46
"659A3259257DD20FB77020F2180D87356EA91B5B97DA92DDEEECF20CA7A96CC017B965525A0F",;47
"60C34FEA4704F3BB3EB53556E576B6BEECE400F16481A5B727D7BCF9344EF5E68C1233EAD4B6",;48
"01D9178B8ACC74ABD11E981FD8B309AEC41F998E777DC4939222DE89BB06E4EC6674A3801AD4",;49
"5ACE6C65E6AA5256B830F7FD4C3D7A1E81AB7577F22153DAFAD65FB91CF90FC478A46A881605",;50
"124D312A633950E8733F4073B0F449D403ACC4BC1F1628B9F7F9CABA855850E4B3FEFED6599C",;51
"985D5E7231E19A2AA7B084EF3B04D3DB438FDB9E08A29F354EA15326EB3A789CC66C77F23151",;52
"C235CB6CCA7C9AEFB759C8BA9E91CD2C3DB7882B148CDB0ED4F7D5F05118B2AC0B297F63D6A2",;53
"A776C9FC3AA42356A06ECF72F32C2D3431B70EC87F03F5E26F9C57F96D6904556EA4DD84A815",;54
"93B9909BE8509DFCFE6691B98BB0808A83EF0F795EC6425781D317598ED46D9E629979357773",;55
"3AA605EC8D9EE5A890DA4CF777DF06D0B32501E44E4A57013EAFA20F94D826B437930689C72F",;56
"89CA74EA95DFC4E1F9A48E9BEBF47D8827CF844873FB15D1CF36C30D00A0D5966D1710A83500",;57
"D31DD8A7A2478F00D55B1F1215692A8A51382EEBF647CCBFFDDA23818FAC3B3B301865B7481F",;58
"6EA2C7E3BF7F111FFA41D38F689E31848B69C07688045FE25D5EE040F5228A0C4D23C02995D2",;59
"DE3DB97F89ABB5A85307C72A3742F1D5EF492B858C77804E7B4BE82105A70E466C937E9DA8AC",;60
"402917C7CFA9B348CE380C01ACC46921FEC9853510554BBD4986DBB78FBFDF355F428F235EF0",;61
"8C322F3A946865326C3E2D5C40C6F181B7A36A70F40552E9DEB509AFE068DAF2B4373BC47DF8",;62
"F40982CBD8CDD837AE8D2920779E330F4FDE7EAF226F53E819DD7AF0AB8A09D30A77A99B516D",;63
"1C75C6E1A5A1FC140083DB946D5BC8FB1B747A4F36A986346535222C1ED55357D438D5648F78",;64
"819DEACA94EB139D6F1E910D41AFC12649D2A1FC3B6988BBFD798049B1A2D6ADA54AB88B571F",;65
"85F2DCA1CBC8501981054E5B784E53FBCC61BD234D4CCD573220400996C2FBA3C82F590EE2C2",;66
"D2C998AC03BE27085E3A743F2F5347E5DE81D6F55CC7AA3D5D1A245145D806DAC4D695471316",;67
"2550B21980C717AB8DD810345446EF8DFAEDD66B3EE2FB19F7BFD6B461468C86FBAC288F64FF",;68
"DA08B37E825C9B1A9B36B6EE4C7D7AB6F34B8299D125B656F2BADE50B42316A7B8A001ED85FF",;69
"9ECE5509D553835862606D0E957548507EAE1B0EFAE642BC06248194981E55D1B8A7C324FC7E",;70
"A41CDD27309D8716C09C4B3F2155421BD0600329BBA9337B1BCA4DE139C6CE01B4540137BDE7",;71
"8E68DECA68DA1A77D43297520F65FBEACED063977395898D1A8F9F674F8CC361C56F12184CFB",;72
"6F49B9A42486A78DE1FB36C7E717A3ADC73B0EA7A6773309AC43397F4C6AF234E06A27E055F7",;73
"0DF121E97C1BC5E0C618338B4CDC84223FBDFD5360BB4A73621D6A1C87A1499B4836B82EA227",;74
"3260083A7A452BDC18B31A1FBDF5CD900835ECFE22D33FBCCAAA35353073B4E8A21808340C88",;75
"1B32D60ADDB4AEE9AA46D0B8F718DCAE8439B19F158A82A5AC9F3012D2A8767B7D5A4C3E6A2C",;76
"425C65DFA86FE814243877424A2E1C72FB988DD4F6A572BED6469B098EE15F7AA9ECFC10AF1D",;77
"0D00":E#_#Section;78
