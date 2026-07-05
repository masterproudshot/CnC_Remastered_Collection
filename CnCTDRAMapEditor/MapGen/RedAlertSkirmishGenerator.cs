//
// Procedural Red Alert skirmish maps (flat terrain v1 + clustered resources).
//
using MobiusEditor.Interface;
using MobiusEditor.Model;
using MobiusEditor.RedAlert;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;

namespace MobiusEditor.MapGen
{
    public sealed class MapGenRecipe
    {
        public string Name { get; set; } = "AIGen_Map";
        public string Theater { get; set; } = "Temperate";
        public int Seed { get; set; } = 1;
        public int Players { get; set; } = 4;
        public int MapSize { get; set; } = 64;
        public double OreDensity { get; set; } = 0.72;
        public double GemDensity { get; set; } = 0.06;
        public int MineCount { get; set; } = 16;
        public int OrePatchCount { get; set; } = 18;
        public int MinOreCellsPerPatch { get; set; } = 64;
        public int MaxOreCellsPerPatch { get; set; } = 320;
        public int GemPatchCount { get; set; } = 10;
        public int MinGemCellsPerPatch { get; set; } = 14;
        public int MaxGemCellsPerPatch { get; set; } = 72;
        public string Briefing { get; set; }
    }

    public static class RedAlertSkirmishGenerator
    {
        public static IList<string> Generate(MapGenRecipe recipe, string outputMprPath)
        {
            var errors = new List<string>();
            if (recipe == null)
            {
                errors.Add("Recipe is null.");
                return errors;
            }
            if (recipe.Players < 2 || recipe.Players > 8)
            {
                errors.Add("Players must be between 2 and 8.");
            }
            if (recipe.MapSize < 16 || recipe.MapSize > 126)
            {
                errors.Add("MapSize must be between 16 and 126.");
            }
            if (errors.Count > 0)
            {
                return errors;
            }

            var random = new Random(recipe.Seed);
            using (var plugin = new GamePlugin(mapImage: false))
            {
                plugin.New(recipe.Theater);

                int margin = (128 - recipe.MapSize) / 2;
                plugin.Map.TopLeft = new Point(margin, margin);
                plugin.Map.Size = new Size(recipe.MapSize, recipe.MapSize);

                FillClearTerrain(plugin.Map);

                var spawnTiles = PlacePlayerWaypoints(plugin.Map, recipe.Players, random);
                ResourcePlacement.Apply(plugin.Map, random, spawnTiles, new ResourcePlacement.Options
                {
                    OreDensity = recipe.OreDensity,
                    GemDensity = recipe.GemDensity,
                    MineCount = recipe.MineCount,
                    SpawnClearRadius = 5,
                    OrePatchCount = recipe.OrePatchCount,
                    MinOreCellsPerPatch = recipe.MinOreCellsPerPatch,
                    MaxOreCellsPerPatch = recipe.MaxOreCellsPerPatch,
                    GemPatchCount = recipe.GemPatchCount,
                    MinGemCellsPerPatch = recipe.MinGemCellsPerPatch,
                    MaxGemCellsPerPatch = recipe.MaxGemCellsPerPatch
                });

                plugin.Map.BasicSection.Name = recipe.Name;
                plugin.Map.BasicSection.SoloMission = false;
                plugin.Map.BasicSection.Brief = "x";
                plugin.Map.BasicSection.Win = "x";
                plugin.Map.BasicSection.Lose = "x";
                plugin.Map.BasicSection.Action = "x";
                plugin.Map.BasicSection.Intro = "x";
                plugin.Map.BasicSection.Theme = "No Theme";
                plugin.Map.BasicSection.Percent = 100;
                plugin.Map.BasicSection.CarryOverMoney = 100;
                plugin.Map.BasicSection.CarryOverCap = -1;

                if (!string.IsNullOrWhiteSpace(recipe.Briefing))
                {
                    plugin.Map.BriefingSection.Briefing = recipe.Briefing;
                }

                errors.AddRange(plugin.CollectValidationErrors());
                if (errors.Count > 0)
                {
                    return errors;
                }

                var dir = Path.GetDirectoryName(outputMprPath);
                if (!string.IsNullOrEmpty(dir) && !Directory.Exists(dir))
                {
                    Directory.CreateDirectory(dir);
                }

                try
                {
                    if (!plugin.SaveMprOnly(outputMprPath, FileType.INI))
                    {
                        errors.Add("MPR save failed (validation).");
                        errors.AddRange(plugin.CollectValidationErrors());
                    }
                    else
                    {
                        errors.AddRange(RedAlertMapSidecars.RepairMpr(outputMprPath));
                    }
                }
                catch (Exception ex)
                {
                    errors.Add("Save exception: " + ex.Message);
                }
            }

            return errors;
        }

        private static void FillClearTerrain(Map map)
        {
            var clearType = map.TemplateTypes.FirstOrDefault(t => t.Equals("clear1"));
            if (clearType == null)
            {
                return;
            }

            foreach (var p in EnumerateRect(map.Bounds))
            {
                map.Templates[p] = new Template { Type = clearType, Icon = 0 };
            }
        }

        private static List<Point> PlacePlayerWaypoints(Map map, int players, Random random)
        {
            var bounds = map.Bounds;
            int inset = 6;
            var corners = new[]
            {
                new Point(bounds.Left + inset, bounds.Top + inset),
                new Point(bounds.Right - inset - 1, bounds.Top + inset),
                new Point(bounds.Left + inset, bounds.Bottom - inset - 1),
                new Point(bounds.Right - inset - 1, bounds.Bottom - inset - 1),
                new Point(bounds.Left + bounds.Width / 2, bounds.Top + inset),
                new Point(bounds.Left + bounds.Width / 2, bounds.Bottom - inset - 1),
                new Point(bounds.Left + inset, bounds.Top + bounds.Height / 2),
                new Point(bounds.Right - inset - 1, bounds.Top + bounds.Height / 2),
            };

            var spawns = new List<Point>();
            for (int i = 0; i < players && i < map.Waypoints.Length; i++)
            {
                var wp = map.Waypoints[i];
                if (wp.Flag != WaypointFlag.PlayerStart)
                {
                    continue;
                }
                var tile = corners[i];
                if (!map.Metrics.GetCell(tile, out int cell))
                {
                    continue;
                }
                wp.Cell = cell;
                spawns.Add(tile);
            }

            return spawns;
        }

        private static IEnumerable<Point> EnumerateRect(Rectangle rect)
        {
            for (int y = rect.Top; y < rect.Bottom; y++)
            {
                for (int x = rect.Left; x < rect.Right; x++)
                {
                    yield return new Point(x, y);
                }
            }
        }
    }
}