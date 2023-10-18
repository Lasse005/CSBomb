# Airsoft CS-bombe

Dette projekt er en open-source airsoft-bombe-simulator inspireret af CSGO og bruger Arduino og forskellige komponenter. Den har flere spiltilstande som Counter Strike og Søg og Ødelæg, hver med to variationer: Kode og Hold. Bomben inkluderer en tilpasselig indstillingsmenu, der tillader hurtige justeringer uden at skulle omprogrammere Arduino'en.

# Funktioner
* Counter Strike Kode/hold
* Søg og ødelæg Kode/Hold
* Nem menu-navigation
* Ændringsbare indstillinger på brættet
* Låsbar menu under kamp

Bemærk, at jeg ikke er en professionel programmør eller designer. Dette projekt er født ud af en passion for sjov og ønsket om at støtte en lokal airsoftbane med en ny bombe-simulator. Koden og 3D-modellerne er måske ikke optimerede eller perfekte, men de "fungerer" til deres tilsigtede formål. Du er velkommen til at tilpasse og justere efter behov for at passe til dine specifikke dele og krav, da dette projekt handler om kreativitet og fornøjelse.

## Indholdsfortegnelse
- [Spiltilstande](#spiltilstand)
    - [Counter Strike](#counter-strike)
        - [Kode](#kode)
        - [Hold](#hold)
    - [Søg og ødelæg](#søg-og-ødelæg)
- [Indstillinger](#indstillinger)
    - [Spiltilstand](#spiltilstand)
    - [Tid til plantning](#tid-til-plantning)
    - [Tid til afmontering](#tid-til-afmontering)
    - [Kodelængde](#kodelængde)
    - [Fejl](#fejl)
    - [Forsinkelse for koder](#forsinkelse-for-koder)
    - [Automatisk tjek](#automatisk-tjek)
    - [Låst menu](#låst-menu)
    - [Summerfrekvens](#summerfrekvens)
    - [Baggrundsbelysning](#baggrundsbelysning)
    - [Gem som standard](#gem-som-standard)
    - [Fabriksindstilling](#fabriksindstilling)
- [Forudindstillinger](#forudindstillinger)
    - [Brugerstandard](#brugerstandard)
    - [GM/TTP/TTD/CL/DFN/M](#gmttpttdcldfnm)

## Spiltilstande
* ### Counter Strike
CS Kode er en spiltilstand som videospillet Counter Strike, hvor der er 2 hold: et angrebshold og et forsvarshold. Målet for angrebsholdet er at plante bomben på et bombested/lokation, hvor forsvarsholdet skal forhindre angriberne i at plante bomben eller afmontere bomben, hvis angrebsholdet har plantet bomben på bombestedet.
* ##### Kode
Kode er en spiltilstand, hvor der genereres en tilfældig kode til bomben. Spillerne skal indtaste denne kode i bomben for at aktivere den. Bombens kode vises i øverste venstre hjørne og afslører et tal ad gangen, indtil hele koden er indtastet, enten for at plante eller afmontere bomben.

```bash
+--------------------+
|X                   |
|DisplayInfo  Timer  |
|             00:00  |
|Y                   |
+--------------------+
```
Timer viser, hvor meget tid der er tilbage.

X er det viste tal i øverste venstre hjørne.

Y er den indtastede kode indtil videre.

DisplayInfo kan vise tekst til spillerne, f.eks. "Bomben er plantet" eller "Forkert kode".

* #### Hold
Hold er en spiltilstand, hvor du skal holde en knap på tastaturet nede for at begynde at aktivere bomben. Det vil vise en fremskridtslinje på bombens display, hvor den langsomt når 100% aktiv, og bomben går i afmonteringsmode, hvor forsvarsholdet skal holde knappen nede for at afmontere den og forhindre angriberholdet i at vinde. Hvis du slipper knappen, vil fremskridtslinjen langsomt gå ned til 0%.

```bash
+--------------------+
|                    |
|DisplayInfo  Timer  |
|             00:00  |
| #############----- |
+--------------------+
```
Timer viser, hvor meget tid der er tilbage.

DisplayInfo kan vise tekst til spillerne, f.eks. "Bomben er plantet" eller "Forkert kode".

### Hovedmenu
- ## Start
  Start er knappen, der starter alt i bomben. Når du trykker på den, tager den dig til en klar-skærm, hvor spillerne kan trykke på en hvilken som helst knap for at starte timeren på bomben, når de hører starttonen fra banen.
  ```bash
  +--------------------+
  |                    |
  |  Tryk på en hvilken |
  |  som helst knap på  |
  |       tastaturet    |
  +--------------------+
  ```
  Ved at trykke på en hvilken som helst knap på tastaturet starter bombens timer. Hvis du vil gå tilbage, kan du altid trykke på knappen for at gå tilbage, hvis du har brug for det.
- ## Info
  Infoskærmen viser nogle grundlæggende oplysninger om bomben med nyttige oplysninger om opsætningen.
  ```bash
  +--------------------+
  |Spiltilstand: CS KODE|
  |Tid til plantning: 10|
  |Tid til afmontering:10|
  |Kodelængde: 4        |
  +--------------------+
  ```
  Kodelængden i infoskærmen kan også vise den tid, det tager at plante bomben i Hold-spiltilstand.
- ## Indstillinger
  Denne menu er, hvor du kan ændre værdierne for bomben med, hvor lang tid du har til at plante eller afmontere bomben, og alt andet for det.

  I indstillingsmenuen, hvis du vil gemme indstillingen som standard, skal du rulle helt ned, indtil du ser

 en knap kaldet "Gem som standard". Dette gør, at de nuværende indstillinger bliver bootet op med disse indstillinger næste gang, du bruger bomben.

  Du kan finde en liste over alle indstillingerne [her](Settings), hvor du kan læse mere om selve indstillingerne.

- ## Forudindstillinger
  Denne menu bruges til nogle hurtige indstillinger for bomben, så du hurtigt kan ændre dem på farten uden at skulle gå ind i indstillingerne og gøre det manuelt.

  Du vil se en liste over elementer med nogle forudindstillinger, formatet det vises i, kalder jeg [GM/TTP/TTD/CL/DFN/M](#gmttpttdcldfnm). Dette betyder Spiltilstand/TidTilPlantning/TidTilAfmontering/KodeLængde/ForsinkelseForKoder/Fejl

- # Indstillinger
* #### Spiltilstand
    Denne indstilling giver dig mulighed for at ændre, hvilken spiltilstand du vil spille på bomben.
    0 = CS Kode,
    1 = CS Hold,
    2 = SAD Kode,
    3 = SAD Hold
* #### Tid til plantning
    Dette er den tid, angrebsholdet har til at plante bomben, inden timeren løber ud, og de vil tabe.
    Denne indstilling kan være 1 minut til 60 minutter lang.
* #### Tid til afmontering
    Dette er den tid, forsvarsholdet har til at afmontere bomben, før den sprænger, og angrebsholdet vil vinde.
    Denne indstilling kan være 1 minut til 60 minutter lang.
* #### Kodelængde
    Denne indstilling ændrer længden af koden på bomben.
    Denne værdi spænder fra 1 til 20 tegn.
* #### Fejl
    Dette er en indstilling, der straffer spillerne, hvis de indtaster den forkerte kode. Den kan slås fra ved at ændre den til 0.
    Denne værdi spænder fra 0 til 60 sekunder.
* #### Forsinkelse for koder
    Denne indstilling sætter en forsinkelse på numrene, før de vises på skærmen for at gøre aktivering og deaktivering af bomben længere.
    Denne værdi spænder fra 0 til 10 sekunder.
* #### Automatisk tjek
    Automatisk tjek er en indstilling, der tjekker koden undervejs, når du forsøger at plante eller afmontere den.
    Denne indstilling kan være TÆNDT eller SLUKKET.
* #### Låst menu
    Låst menu er en vigtig indstilling, hvis du ikke vil have, at spillere går ind i menuen ved et uheld eller ændrer værdier ved en fejl.
    Hvis TÆNDT, kan du ikke trykke på knappen for at gå tilbage i menuen, men du skal trykke på * og # samtidigt for at låse bomben op igen.
    Denne værdi kan være TÆNDT eller SLUKKET.
* #### Summerfrekvens
    Summerfrekvensen kan være fra 0 til 255, og den ændrer summerens frekvens, så brugeren af bomben kan vælge en lyd, de kan lide :P
    Denne værdi spænder fra 0 til 255.
* #### Baggrundsbelysning
    Baggrundsbelysning giver dig mulighed for at tænde eller slukke for baggrundsbelysningen på LCD-skærmen.
    Denne værdi kan være TÆNDT eller SLUKKET.
* #### Gem som standard
    Gem som standard vil gemme alle indstillingerne på bomben lige nu i dens hukommelse og indlæse dem, når bomben startes, og have dem i forudindstillingsmenuen.
* #### Fabriksindstilling
    Fabriksindstilling vil nulstille alle indstillingerne til de oprindelige standardindstillinger.

# Forudindstillinger
Forudindstillinger er en menu med forudindstillede indstillinger for spiltilstande, hvor det er en hurtig måde at ændre indstillinger på uden at skulle gå ind i indstillingerne og gøre alt manuelt.
* #### Brugerstandard
Bruger standardindstillingerne, der bruges ved opstart.
* #### GM/TTP/TTD/CL/DFN/M
Dette er det format, jeg bruger til at sige forudindstillingerne uden at skrive alt ud,
GM = Spiltilstand,
TTP = Tid til plantning,
TTD = Tid til afmontering,
CL = Kodelængde,
DFN = Forsinkelse for koder,
M = Fejl