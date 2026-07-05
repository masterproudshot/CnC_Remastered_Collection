//
// Headless initialization of map editor DATA (MEG tilesets) for CLI map generation.
//
using MobiusEditor.Utility;
using System;
using System.Globalization;
using System.IO;
using System.Threading;

namespace MobiusEditor.MapGen
{
    public static class EditorDataHost
    {
        public static bool TryInitialize(string gameRootDirectory, out string errorMessage)
        {
            errorMessage = null;

            if (string.IsNullOrWhiteSpace(gameRootDirectory) || !Directory.Exists(gameRootDirectory))
            {
                errorMessage = "Game root directory does not exist: " + gameRootDirectory;
                return false;
            }

            if (Thread.CurrentThread.CurrentCulture.Name != "en-US")
            {
                Thread.CurrentThread.CurrentCulture = CultureInfo.CreateSpecificCulture("en-US");
            }

            Globals.TheMegafileManager = new MegafileManager(gameRootDirectory);

            var megafilePath = Path.Combine(gameRootDirectory, "DATA");
            var megafilesLoaded = true;
            megafilesLoaded &= Globals.TheMegafileManager.Load(Path.Combine(megafilePath, "CONFIG.MEG"));
            megafilesLoaded &= Globals.TheMegafileManager.Load(Path.Combine(megafilePath, "TEXTURES_COMMON_SRGB.MEG"));
            megafilesLoaded &= Globals.TheMegafileManager.Load(Path.Combine(megafilePath, "TEXTURES_RA_SRGB.MEG"));
            megafilesLoaded &= Globals.TheMegafileManager.Load(Path.Combine(megafilePath, "TEXTURES_SRGB.MEG"));
            megafilesLoaded &= Globals.TheMegafileManager.Load(Path.Combine(megafilePath, "TEXTURES_TD_SRGB.MEG"));

            if (!megafilesLoaded)
            {
                errorMessage = "Required DATA MEG files missing under: " + megafilePath;
                return false;
            }

            Globals.TheTextureManager = new TextureManager(Globals.TheMegafileManager);
            Globals.TheTilesetManager = new TilesetManager(
                Globals.TheMegafileManager,
                Globals.TheTextureManager,
                Globals.TilesetsXMLPath,
                Globals.TexturesPath);
            Globals.TheTeamColorManager = new TeamColorManager(Globals.TheMegafileManager);
            Globals.TheTeamColorManager.Load(@"DATA\XML\CNCRATEAMCOLORS.XML");

            var cultureName = CultureInfo.CurrentUICulture.Name;
            var gameTextFilename = string.Format(Globals.GameTextFilenameFormat, cultureName.ToUpper());
            if (!Globals.TheMegafileManager.Exists(gameTextFilename))
            {
                gameTextFilename = string.Format(Globals.GameTextFilenameFormat, "EN-US");
            }
            Globals.TheGameTextManager = new GameTextManager(Globals.TheMegafileManager, gameTextFilename);

            return true;
        }
    }
}