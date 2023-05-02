#!/usr/bin/python


import os


print("*** Quel mac? ***")
computerid=raw_input('DAQ computer ID (f, g, h, s, x): ')
if computerid in ['f','g','h','s','x']:
    print("")
else:
    print("Unknown ID "+computerid)
    exit()



print("*** CREATE CABLING ***")
while 1:
    cabling=raw_input('Cabling name (e.g. signal1234): ')
    detector="det"+cabling
    #echo $cabling
    if(len(cabling)<2):
        print("name too short")
        continue
    command="grep ' "+cabling+" := detecteur' modeles_SPC/cablages > /dev/null"
    #print(command[:-12])
    #os.system(command[:-12])
    test=os.system(command)
    command2="grep ' "+detector+" := capteurs' modeles_SPC/detecteurs > /dev/null"
    test2=os.system(command2)
    if test==0:
	    print("Cabling name '"+cabling+"' already exists in modeles_SPC/cablages")
    elif test2==0:
	    print("Detector name '"+detector+"' already exists in modeles_SPC/detecteurs")
    else:
        break

print(cabling+", "+detector)
#    fi


channels = []

print("*** SELECT/CREATE CHANNELS ***")
while 1:
    detfile = open("modeles_SPC/detecteurs","r")
    start=0
    newchannel=raw_input('Add channel? (y/n): ')
    if newchannel != 'y':
        break
    print("Existing channels:")
    sensorlist = []
    for line in detfile:
        line=line.strip()
        #print line
        if line.startswith('Voies'):
            start=1
            continue
        if line.startswith('Detecteurs'):
            break
        if start == 0:
            continue
        if line.startswith('{ '):
            #print "XXXXX"
            line=line[2:]
            splitline=line.split(' ')
            sensorlist.append(splitline[0])
            print("  "+sensorlist[len(sensorlist)-1])
    detfile.close()
    channel=raw_input('Channel name: ')
    print('Channel colour R/G/B in %:')
    while 1:
        print('e.g. red: 100/0/0, white: 100/100/100')
        col=raw_input('')
        cols=col.split('/')
        if len(cols)==3:
            red=int(cols[0])
            green=int(cols[1])
            blue=int(cols[2])
            break
        else:
            print('invalid format')
            continue
    width=raw_input('Window width in ms (default 2): ')
    if len(width)<=0:
        width = 2
    delay=raw_input('Delay wrt trigger (default 0): ')
    if len(delay)<=0:
        delay = 0
    search=raw_input('Search window in ms (default 2): ')
    if len(search)<=0:
        search = 2
    pretrig=raw_input('pretrigger in % (default 50): ')
    if len(pretrig)<=0:
        pretrig = 50
    deadtime=raw_input('Dead time after event in ms (default 1): ')
    if len(deadtime)<=0:
        deadtime = 1


    channels.append({'name':channel,
                    'R':red,
                    'G':green,
                    'B':blue,
                    'width':width,
                    'delay':delay,
                    'search':search,
                    'pretrig':pretrig,
                    'deadtime':deadtime})
    print(channel+" : "+str(red)+'/'+str(green)+'/'+str(blue))

    if channel in sensorlist:
        print("Using existing channel "+channel)
        os.system("grep ' "+channel+" := phys' modeles_SPC/detecteurs")
    else:    ##### UPDATE FILE modeles_SPC/detecteurs ####
        newdetfile = open("modeles_SPC/detecteurs.tmp","w")
        detfile = open("modeles_SPC/detecteurs","r")
        for line in detfile:
            newdetfile.write(line)
            if line.startswith('Voies'):
                newdetfile.write('        { '+channel+' := physique = charge, temps-mort-ms = -1, pretrigger_% = 25, base.debut_% = 25, base.fin_% = 75, montee.debut_% = 10, montee.fin_% = 90, coincidence-ms = 20 },'+"\n")
        detfile.close()
        newdetfile.close()
        os.system("mv modeles_SPC/detecteurs.tmp modeles_SPC/detecteurs")
if len(channels)<1:
    print("You need to define 1 to 4 channels!")
    exit()

dirname="setup_"+cabling
os.system("cp -r setup_template "+dirname)



filecomputer = open(dirname+"/Hardware/Computers","w")
filecomputer.write("# Reseau participant a l'acquisition\n")
filecomputer.write("#\n")
filecomputer.write("acquisition = { local := ip = ., runs = "+computerid+", repartiteurs = CALI00 }\n")
filecomputer.close()

#############################################
##### UPDATE FILE modeles_SPC/detecteurs ####
#############################################
newdetfile = open("modeles_SPC/detecteurs.tmp2","w")
detfile = open("modeles_SPC/detecteurs","r")
for line in detfile:
    newdetfile.write(line)
    if line.startswith('Detecteurs'):
        newline = '        { '+ detector + ' := capteurs = ('
        first = 1
        for ch in channels:
            if first == 1:
                first = 0
            else:
                newline += ', '
            newline += ch['name']
        newline += '), reglages = (), associes = () },'
        newdetfile.write(newline+"\n")
detfile.close()
newdetfile.close()
os.system("mv modeles_SPC/detecteurs.tmp2 modeles_SPC/detecteurs")

#############################################
##### UPDATE FILE modeles_SPC/cablages ####
#############################################
newfilecablage = open("modeles_SPC/cablages.tmp","w")
filecablage = open("modeles_SPC/cablages","r")
for line in filecablage:
    newfilecablage.write(line)
    if line.startswith('Cablages'):
        newfilecablage.write("    { "+cabling+" := detecteur = "+detector+", numeriseurs = (AdcCali),\n")
        newline = '        capteurs = ( '
        count = 1
        for ch in channels:
            if count> 1:
                newline += ', '
            newline += '{ ' + ch['name'] + ' := 1.' + str(count) + ' }'
            count += 1
        newline += ' ),'
        newfilecablage.write(newline+"\n")
        newfilecablage.write("        reglages = ()\n")
        newfilecablage.write("    },\n")
filecablage.close()
newfilecablage.close()
os.system("mv modeles_SPC/cablages.tmp modeles_SPC/cablages")







#############################################
##### CREATE FILE setup/Hardware/Cabling ####
#############################################
filecabling=open(dirname+"/Hardware/Cabling",'w')
cables=""
count=1
for ch in channels:
    if count>1:
        cables += ", "
    cables += "01."+str(count)
    count += 1
filecabling.write('/'+cabling+': a1 > '+str(cables)+"\n")
filecabling.close()



#############################################
##### CREATE FILE setup/Gene ####
#############################################
filegene=open(dirname+"/Gene",'w')
filegene.write("#\n")
filegene.write("#\n")
filegene.write('Generateurs = {  := voies = "'+channels[0]['name']+' sphere", niveau_DC = 0, bruit = 5, frequence_evt = 1, energie_ADU = 1000, resolution = 10 }'+"\n")
filegene.close()



######################################################
##### CREATE FILE setup/Hardware/Detectors/Connexions ####
######################################################
fileconnection=open(dirname+"/Hardware/Detectors/Connexions",'w')
fileconnection.write("a1 > @sphere_"+cabling)
fileconnection.close()

######################################################
##### CREATE FILE setup/Hardware/Detectors/sphere ####
######################################################
filedet=open(dirname+"/Hardware/Detectors/sphere_"+cabling,'w')
filedet.write("#\n")
filedet.write("sphere/"+detector+": ok (HS/ok)\n")
filedet.write("        parms detecteur = {\n")
filedet.write("                masse = 0 	# masse du detecteur (g)\n")
filedet.write("                mode = standard 	# mode de sauvegarde\n")
filedet.write("                demarrage = neant 	# nom du fichier de demarrage de run\n")
filedet.write("                entretien = neant 	# nom du fichier d'entretien periodique\n")
filedet.write("                voisins = () 	# detecteurs a sauver aussi en mode 'voisins'\n")
filedet.write("                associes = () 	# detecteurs a sauver aussi en mode 'individuel'\n")
filedet.write("        }\n")
for ch in channels:
    filedet.write("#\n")
    filedet.write("#	---------- Parametrage de la voie "+ch['name']+" ----------\n")
    filedet.write("#\n")
    filedet.write("	parms "+ch['name']+" = {\n")
    filedet.write("		usage = defaut 	# Utilisation finale de la voie (jetee/defaut/stream/evts/tout)\n")
    filedet.write("		sensibilite = 1 	# nV/keV\n")
    filedet.write("		Rdetecteur = 1.4013e-45 	# Mohms\n")
    filedet.write("	}\n")
    filedet.write("	evenement "+ch['name']+" = {\n")
    filedet.write("		duree = "+str(ch['width'])+" 	# Duree evenement (ms)\n")
    filedet.write("		delai = "+str(ch['delay'])+" 	# Decalage evenement (ms)\n")
    filedet.write("		interv = "+str(ch['search'])+" 	# Fenetre de recherche (ms)\n")
    filedet.write("		pre-trigger = "+str(ch['pretrig'])+" 	# Pre-trigger (%)\n")
    filedet.write("		temps-mort = "+str(ch['deadtime'])+" 	# Temps mort avant nouvelle recherche (milllisec.) (=post-trigger si <0)\n")
    filedet.write("		base.depart = 20 	# Position du debut de ligne de base dans le pre-trigger (%)\n")
    filedet.write("		base.arrivee = 60 	# Position de la fin de ligne de base dans le pre-trigger (%)\n")
    filedet.write("		temps.depart = 10 	# Debut du temps de montee (%)\n")
    filedet.write("		temps.arrivee = 90 	# Fin du temps de montee (%)\n")
    filedet.write("		phase1.t0 = 0 	# Debut integrale rapide (ms)\n")
    filedet.write("		phase1.dt = 0 	# Duree integrale rapide (ms)\n")
    filedet.write("		phase2.t0 = 0 	# Debut integrale longue (ms)\n")
    filedet.write("		phase2.dt = 0 	# Duree integrale longue (ms)\n")
    filedet.write("		template.dim = 0 	# Dimension du template (points)\n")
    filedet.write("		template.temps.montee = 1 	# temps de montee (ms)\n")
    filedet.write("		template.temps.descente1 = 100 	# temps de la descente #1 (ms)\n")
    filedet.write("		template.temps.descente2 = 300 	# temps de la descente #2 (ms)\n")
    filedet.write("		template.facteur.descente2 = 0.5 	# gain de la descente #2\n")
    filedet.write("		moyen.min = 1000 	# Amplitude min pour calcul evt moyen\n")
    filedet.write("		moyen.max = 8000 	# Amplitude max pour calcul evt moyen\n")
    filedet.write("		affichage.min = -8192 	# Signal recu minimum (ADU) [pour l'affichage]\n")
    filedet.write("		affichage.max = 8191 	# Signal recu maximum (ADU) [pour l'affichage]\n")
    filedet.write("	}\n")
    filedet.write("	trmt-au-vol "+ch['name']+" = {\n")
    filedet.write("		type = neant 	# Traitement au vol (neant/demodulation/filtrage/invalidation)\n")
    filedet.write("		parametre = 21 	# Parametre (int) du traitement au vol\n")
    filedet.write('		texte = "sur 21 pts" 	# Parametre (char[]) du traitement au vol'+"\n")
    filedet.write("		gain = 1 	# Gain logiciel au vol\n")
    filedet.write("		execution = au_vol 	# La demodulation peut etre differee (au_vol/sur_tampon)\n")
    filedet.write("	}\n")
    filedet.write("	trmt-sur-tampon "+ch['name']+" = {\n")
    filedet.write("		type = lissage 	# Traitement pre-trigger (neant/demodulation/filtrage/invalidation)\n")
    filedet.write("		parametre = 11 	# Parametre (int) du traitement pre-trigger\n")
    filedet.write('		texte = "sur 11 pts" 	# Parametre (char[]) du traitement pre-trigger'+"\n")
    filedet.write("		gain = 1 	# Gain logiciel pre-trigger\n")
    filedet.write("		dim.tampon = 10000000 	# Dimension du tampon resultat des traitements\n")
    filedet.write("		dim.lissage = 100 	# Dimension de l'evaluation de la ligne de base\n")
    filedet.write("		periodes.pattern = 0 	# Nb.periodes pour soustraction du pattern\n")
    filedet.write("		moyenne.archive = 0 	# Facteur de moyennage avant la sauvegarde des evenements\n")
    filedet.write("		template = neant 	# Filtrage par template (neant/database/analytique)\n")
    filedet.write("	}\n")
    filedet.write("	trigger "+ch['name']+" = {\n")
    filedet.write("		type = neant 	# Type de recherche d'evenement (neant/seuil/front/porte/bruit)\n")
    filedet.write("		signe = positif 	# Sens de la variation attendue (negatif/indifferent/positif)\n")
    filedet.write("		origine = interne 	# Origine du trigger (interne/deporte/fpga)\n")
    filedet.write("		regulation = non 	# Autorisation de reguler automatiquement\n")
    filedet.write("		reprise = fixe 	# Tactique de gestion du temps mort apres evenement (fixe/attends_seuil/reporte)\n")
    filedet.write("		amplitude.min = 100 	# Amplitude minimum pour impulsions positives (ADU)\n")
    filedet.write("		amplitude.max = 23000 	# Amplitude maximum pour impulsions negatives (ADU)\n")
    filedet.write("		montee = 0.015 	# Temps de montee minimum (microsecs.)\n")
    filedet.write("		duree = 10 	# Duree d'evenement minimum (microsecs.)\n")
    filedet.write("		#		Coupures avant sauvegarde\n")
    filedet.write("		coupures = non 	# Vrai si coupures actives (non/oui/toujours)\n")
    filedet.write("		veto = non 	# Vrai si voie en veto\n")
    filedet.write("		ampl.rejette = non 	# Inverse la condition de coupure sur l'amplitude\n")
    filedet.write("		underflow = 50 	# Plancher pour les amplitudes (ADU)\n")
    filedet.write("		overflow = 50000 	# Plafond pour les amplitudes (ADU)\n")
    filedet.write("		montee.rejette = non 	# Inverse la condition de coupure sur le temps de montee\n")
    filedet.write("		montee.min = 0 	# Temps de montee minimum (millisecs)\n")
    filedet.write("		montee.max = 0.004 	# Temps de montee maximum (millisecs)\n")
    filedet.write("		bruit.rejette = non 	# Inverse la condition de coupure sur le bruit\n")
    filedet.write("		bruit.max = 1e+06 	# Bruit maximum sur la ligne de base (ADU)\n")
    filedet.write("		alpha.amplitude = 80000 	# Seuil pour un temps mort special alpha (ADU)\n")
    filedet.write("		alpha.duree = 5000 	# Temps mort special alpha (ms)\n")
    filedet.write("		#		Interconnexion entre voies\n")
    filedet.write("		connexion.out = non 	# Autorisation de sortie\n")
    filedet.write("		connexion.delai = 0 	# Delai de sortie du signal trigger [-1 si pas emis] (millisecs)\n")
    filedet.write("		connexion.in = non 	# Autorisation d'entree\n")
    filedet.write("		connexion.detec = 0 	# Detecteur branche sur sortie trigger autre voie\n")
    filedet.write("		connexion.capteur = 0 	# Capteur (dudit detecteur) branche sur sortie trigger autre voie\n")
    filedet.write("	}\n")
    filedet.write("	regulation "+ch['name']+" = {\n")
    filedet.write("		Type = neant 	# Type de regulation (neant/taux/intervalles)\n")
    filedet.write("		Plancher.min = 0 	# Plancher pour le seuil min\n")
    filedet.write("		Plafond.min = 0 	# Plafond  pour le seuil min\n")
    filedet.write("		Plancher.max = 0 	# Plancher pour le seuil max\n")
    filedet.write("		Plafond.max = 0 	# Plafond  pour le seuil max\n")
    filedet.write("		#\n")
    filedet.write("		#		Regulation par le taux d'evenements\n")
    filedet.write("		#\n")
    filedet.write("		Active.1 = non 	# Echelle de temps #1 en service\n")
    filedet.write("		Echelle.1 = 0 	# Echelle de temps #1 (mn)\n")
    filedet.write("		Nb.1.min = 0 	# Nombre d'evt minimum a l'echelle #1\n")
    filedet.write("		Action.1.min.type = continuer 	# Type action si nb1 < min1\n")
    filedet.write("		Action.1.min.parm = 0 	# Valeur action si nb1 < min1\n")
    filedet.write("		Nb.1.max = 0 	# Nombre d'evt maximum a l'echelle #1\n")
    filedet.write("		Action.1.max.type = continuer 	# Type action si nb1 > max1\n")
    filedet.write("		Action.1.max.parm = 0 	# Valeur action si nb1 > max1\n")
    filedet.write("		#\n")
    filedet.write("		Active.2 = non 	# Echelle de temps #2 en service\n")
    filedet.write("		Echelle.2 = 0 	# Echelle de temps #2 (mn)\n")
    filedet.write("		Nb.2.min = 0 	# Nombre d'evt minimum a l'echelle #2\n")
    filedet.write("		Action.2.min.type = continuer 	# Type action si nb2 < min2\n")
    filedet.write("		Action.2.min.parm = 0 	# Valeur action si nb2 < min2\n")
    filedet.write("		Nb.2.max = 0 	# Nombre d'evt maximum a l'echelle #2\n")
    filedet.write("		Action.2.max.type = continuer 	# Type action si nb2 > max2\n")
    filedet.write("		Action.2.max.parm = 0 	# Valeur action si nb2 > max2\n")
    filedet.write("		#\n")
    filedet.write("		Active.3 = non 	# Echelle de temps #3 en service\n")
    filedet.write("		Echelle.3 = 0 	# Echelle de temps #3 (mn)\n")
    filedet.write("		Nb.3.min = 0 	# Nombre d'evt minimum a l'echelle #3\n")
    filedet.write("		Action.3.min.type = continuer 	# Type action si nb3 < min3\n")
    filedet.write("		Action.3.min.parm = 0 	# Valeur action si nb3 < min3\n")
    filedet.write("		Nb.3.max = 0 	# Nombre d'evt maximum a l'echelle #3\n")
    filedet.write("		Action.3.max.type = continuer 	# Type action si nb3 > max3\n")
    filedet.write("		Action.3.max.parm = 0 	# Valeur action si nb3 > max3\n")
    filedet.write("		#\n")
    filedet.write("		#		Regulation par intervalles\n")
    filedet.write("		#\n")
    filedet.write("		Interv.nb = 0 	# Nombre d'intervalles\n")
    filedet.write("		Interv.duree = 0 	# Longueur (s)\n")
    filedet.write("		Interv.ajuste = 0 	# Ajustement seuil (ADU)\n")
    filedet.write("		Interv.evt = 0 	# Facteur decision\n")
    filedet.write("		Interv.min = 0 	# Delai mini (nb.interv.)\n")
    filedet.write("		Interv.incr = 0 	# Increment si bruit (ADU)\n")
    filedet.write("	}\n")

filedet.close()





########################################################
##### CREATE FILE setup/Plots (plots configuration) ####
########################################################
fileplots = open(dirname+"/Plots","w")

signal = {
    "high amplitude sensor": [-33000,33000,20,344,359,17,365,"oui","non"],
    "low amplitude sensor": [-3000,3000,20,344,359,17,365,"non","non"],
}
fileplots.write('# [claps]'+"\n")
fileplots.write("Fenetres = ("+"\n")
for window in signal:
    fileplots.write('   { "'+window+'" := signal, min = '+str(signal[window][0])+', max = '+str(signal[window][1])+', duree_ms = '+str(signal[window][2])+', donnees = ('+"\n")
    for ch in channels:
        sensor = ch['name']
        fileplots.write('      { capteur = '+sensor+', detecteur = sphere, etat = brute, couleur = { rouge = '+str(ch['R'])+', vert = '+str(ch['G'])+', bleu = '+str(ch['B'])+' } },'+"\n")
    fileplots.write('   ), dimension = { l = '+str(signal[window][3])+', h = '+str(signal[window][4])+' }, position = { x = '+str(signal[window][5])+', y = '+str(signal[window][6])+' }, affichage = '+str(signal[window][7])+', grille = '+str(signal[window][8])+' },'+"\n")


histo = {
    "baseline RMS": ["bruit",2,60,200,0,344,259,32,674,"oui","non"],
    "high amplitudes": ["amplitude",3000,40000,200,0,344,259,910,703,"oui","non"],
    "low amplitudes": ["amplitude",0,3000,200,0,344,259,475,703,"oui","non"],
    "risetime": ["tps.montee",0,0.1,100,0,344,259,1025,45,"oui","non"],
    "baseline": ["base",-40000,40000,100,0,344,259,1025,285,"non","non"],
}

for window in histo:
    fileplots.write('   { "'+window+'" := histo, bin_min = '+str(histo[window][1])+', bin_max = '+str(histo[window][2])+', nb.bins = '+str(histo[window][3])+', max.evts = '+str(histo[window][4])+', donnees = ('+"\n")
    for ch in channels:
        sensor = ch['name']
        fileplots.write('      { '+str(histo[window][0])+',capteur = '+sensor+', detecteur = sphere, couleur = { rouge = '+str(ch['R'])+', vert = '+str(ch['G'])+', bleu = '+str(ch['B'])+' } },'+"\n")
    fileplots.write('   ), dimension = { l = '+str(histo[window][5])+', h = '+str(histo[window][6])+' }, position = { x = '+str(histo[window][7])+', y = '+str(histo[window][8])+' }, affichage = '+str(histo[window][9])+', grille = '+histo[window][10]+' },'+"\n")

fileplots.write('   { "FFT" := FFT, nb.points = 100000, freq_min = 0.01, freq_max = 520.833, bruit_min = 0, bruit_max = 0, intervalles = 1, axe_X = log, hors_evts = non, donnees = ('+"\n")
for ch in channels:
    sensor = ch['name']
    fileplots.write('      { capteur = '+sensor+', detecteur = sphere, etat = brute, couleur = { rouge = '+str(ch['R'])+', vert = '+str(ch['G'])+', bleu = '+str(ch['B'])+' } },'+"\n")
fileplots.write('   ), dimension = { l = 418, h = 286 }, position = { x = 882, y = 45 }, affichage = non, grille = non },'+"\n")

fileplots.write(')'+"\n")
fileplots.close()
