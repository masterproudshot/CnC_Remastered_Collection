//
// Command-line entry: CnCTDRAMapEditorD.exe --mapgen generate|repair-previews ...
//
using MobiusEditor.RedAlert;
using Globals = MobiusEditor.Globals;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MobiusEditor.MapGen
{
    public static class MapGenCli
    {
        private const string DefaultGameRoot = @"C:\Program Files (x86)\Steam\steamapps\common\CnCRemastered";

        public static int Run(string[] args)
        {
            WriteLog("MapGenCli.Run argc=" + (args?.Length ?? 0));

            if (args.Length < 2 || !string.Equals(args[0], "--mapgen", StringComparison.OrdinalIgnoreCase))
            {
                PrintUsage();
                return 2;
            }

            var command = args[1].ToLowerInvariant();
            if (command == "repair-previews")
            {
                return RunRepairPreviews(args);
            }
            if (command != "generate")
            {
                PrintUsage();
                return 2;
            }

            var recipe = new MapGenRecipe();
            string gameRoot = DefaultGameRoot;
            string outputPath = null;

            for (int i = 2; i < args.Length; i++)
            {
                var arg = args[i];
                if (arg.StartsWith("--", StringComparison.Ordinal))
                {
                    var key = arg.Substring(2).ToLowerInvariant();
                    if (i + 1 >= args.Length)
                    {
                        Console.Error.WriteLine("Missing value for " + arg);
                        return 2;
                    }
                    var value = args[++i];
                    switch (key)
                    {
                        case "name": recipe.Name = value; break;
                        case "theater": recipe.Theater = value; break;
                        case "seed": recipe.Seed = int.Parse(value); break;
                        case "players": recipe.Players = int.Parse(value); break;
                        case "size": recipe.MapSize = int.Parse(value); break;
                        case "ore": recipe.OreDensity = double.Parse(value); break;
                        case "gems": recipe.GemDensity = double.Parse(value); break;
                        case "mines": recipe.MineCount = int.Parse(value); break;
                        case "ore-patches": recipe.OrePatchCount = int.Parse(value); break;
                        case "ore-patch-min": recipe.MinOreCellsPerPatch = int.Parse(value); break;
                        case "ore-patch-max": recipe.MaxOreCellsPerPatch = int.Parse(value); break;
                        case "gem-patches": recipe.GemPatchCount = int.Parse(value); break;
                        case "gem-patch-min": recipe.MinGemCellsPerPatch = int.Parse(value); break;
                        case "gem-patch-max": recipe.MaxGemCellsPerPatch = int.Parse(value); break;
                        case "briefing": recipe.Briefing = value; break;
                        case "data": gameRoot = value; break;
                        case "out": outputPath = value; break;
                        default:
                            Console.Error.WriteLine("Unknown option: " + arg);
                            return 2;
                    }
                }
            }

            if (string.IsNullOrWhiteSpace(outputPath))
            {
                var saveDir = Constants.SaveDirectory;
                if (!Directory.Exists(saveDir))
                {
                    Directory.CreateDirectory(saveDir);
                }
                outputPath = Path.Combine(saveDir, SanitizeFileName(recipe.Name) + ".mpr");
            }

            if (!EditorDataHost.TryInitialize(gameRoot, out string initError))
            {
                WriteLog("INIT FAILED: " + initError);
                return 1;
            }

            var previousDirectory = Directory.GetCurrentDirectory();
            try
            {
                Directory.SetCurrentDirectory(gameRoot);
                Globals.SkipTypeThumbnailInit = true;
                IList<string> errors = RedAlertSkirmishGenerator.Generate(recipe, outputPath);
                Globals.SkipTypeThumbnailInit = false;
                if (errors.Count > 0)
                {
                    WriteLog("FAILED:");
                    foreach (var e in errors)
                    {
                        WriteLog("  " + e);
                    }
                    return 1;
                }

                WriteLog("OK: " + outputPath);
                return 0;
            }
            finally
            {
                Directory.SetCurrentDirectory(previousDirectory);
            }
        }

        private static int RunRepairPreviews(string[] args)
        {
            string gameRoot = DefaultGameRoot;
            string dir = null;
            string singleMpr = null;

            for (int i = 2; i < args.Length; i++)
            {
                var arg = args[i];
                if (!arg.StartsWith("--", StringComparison.Ordinal))
                {
                    continue;
                }
                var key = arg.Substring(2).ToLowerInvariant();
                if (i + 1 >= args.Length)
                {
                    Console.Error.WriteLine("Missing value for " + arg);
                    return 2;
                }
                var value = args[++i];
                switch (key)
                {
                    case "data": gameRoot = value; break;
                    case "dir": dir = value; break;
                    case "mpr": singleMpr = value; break;
                    default:
                        Console.Error.WriteLine("Unknown option: " + arg);
                        return 2;
                }
            }

            if (string.IsNullOrWhiteSpace(dir) && string.IsNullOrWhiteSpace(singleMpr))
            {
                dir = Constants.SaveDirectory;
            }

            if (!EditorDataHost.TryInitialize(gameRoot, out string initError))
            {
                WriteLog("INIT FAILED: " + initError);
                return 1;
            }

            var previousDirectory = Directory.GetCurrentDirectory();
            try
            {
                Directory.SetCurrentDirectory(gameRoot);
                Globals.SkipTypeThumbnailInit = true;
                return RunRepairPreviewsCore(dir, singleMpr);
            }
            finally
            {
                Globals.SkipTypeThumbnailInit = false;
                Directory.SetCurrentDirectory(previousDirectory);
            }
        }

        private static int RunRepairPreviewsCore(string dir, string singleMpr)
        {
            var mprFiles = new List<string>();
            if (!string.IsNullOrWhiteSpace(singleMpr))
            {
                mprFiles.Add(singleMpr);
            }
            if (!string.IsNullOrWhiteSpace(dir) && Directory.Exists(dir))
            {
                mprFiles.AddRange(Directory.EnumerateFiles(dir, "*.mpr", SearchOption.TopDirectoryOnly));
                mprFiles.AddRange(Directory.EnumerateFiles(dir, "*.MPR", SearchOption.TopDirectoryOnly));
            }

            mprFiles = mprFiles.Distinct(StringComparer.OrdinalIgnoreCase).ToList();
            if (mprFiles.Count == 0)
            {
                WriteLog("repair-previews: no .mpr files found.");
                return 1;
            }

            int failed = 0;
            foreach (var mpr in mprFiles)
            {
                var errors = RedAlertMapSidecars.RepairMpr(mpr);
                if (errors.Count > 0)
                {
                    failed++;
                    WriteLog("REPAIR FAIL " + mpr);
                    foreach (var e in errors)
                    {
                        WriteLog("  " + e);
                    }
                }
                else
                {
                    WriteLog("REPAIR OK " + mpr);
                }
            }

            return failed > 0 ? 1 : 0;
        }

        private static void WriteLog(string message)
        {
            try
            {
                var logPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "mapgen.log");
                File.AppendAllText(logPath, DateTime.Now.ToString("o") + " " + message + Environment.NewLine);
            }
            catch
            {
            }
        }

        private static string SanitizeFileName(string name)
        {
            foreach (var c in Path.GetInvalidFileNameChars())
            {
                name = name.Replace(c, '_');
            }
            return string.IsNullOrWhiteSpace(name) ? "AIGen_Map" : name.Trim();
        }

        private static void PrintUsage()
        {
            Console.WriteLine("Usage:");
            Console.WriteLine("  CnCTDRAMapEditorD.exe --mapgen generate --name MyMap --seed 42 --players 4 \\");
            Console.WriteLine("    --size 64 --ore 0.58 --gems 0.045 --mines 14 [--ore-patches 14] [--data <CnCRemastered>] [--out <path.mpr>]");
            Console.WriteLine("  CnCTDRAMapEditorD.exe --mapgen repair-previews --dir <Red_Alert folder> [--data <CnCRemastered>]");
            Console.WriteLine("  CnCTDRAMapEditorD.exe --mapgen repair-previews --mpr <file.mpr> [--data <CnCRemastered>]");
        }
    }
}