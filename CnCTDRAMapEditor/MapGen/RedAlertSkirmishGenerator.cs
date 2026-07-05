//
// Procedural Red Alert skirmish maps (flat terrain v1 + temperate-mixed v2 + clustered resources).
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
        /// <summary>B1 terrain profile: flat (v1), temperate-mixed (v2).</summary>
        public string TerrainProfile { get; set; } = MapGenTerrainProfiles.Flat;
        /// <summary>B1 spawn layout: corners8, octagonOpen, middleRoad.</summary>
        public string SpawnLayout { get; set; } = MapGenSpawnLayouts.Corners8;
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
            var validationErrors = MapGenRecipeValidation.ValidateAndResolve(recipe, out SpawnLayoutResolution layout);
            var errors = new List<string>(validationErrors);
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

                var spawnTilesForTerrain = MapGenSpawnTiles.Resolve(plugin.Map, recipe.Players, layout);
                TerrainPlacement.Apply(plugin.Map, random, spawnTilesForTerrain, new TerrainPlacement.Options
                {
                    TerrainProfile = recipe.TerrainProfile,
                    SpawnPadRadius = 5
                });

                var spawnTiles = PlacePlayerWaypoints(plugin.Map, recipe.Players, layout);
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
                        errors.AddRange(RedAlertMapSidecars.WriteSidecars(
                            plugin.Map,
                            outputMprPath,
                            RedAlertMapSidecars.MetadataFromRecipe(recipe, layout)));
                    }
                }
                catch (Exception ex)
                {
                    errors.Add("Save exception: " + ex.Message);
                }
            }

            return errors;
        }

        private static List<Point> PlacePlayerWaypoints(Map map, int players, SpawnLayoutResolution layout)
        {
            var spawns = new List<Point>();
            if (layout.UsesReferenceCells)
            {
                return PlaceReferenceWaypoints(map, players, layout.ReferenceCells, spawns);
            }

            return PlaceCorners8Waypoints(map, players, spawns);
        }

        private static List<Point> PlaceCorners8Waypoints(Map map, int players, List<Point> spawns)
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

            AssignWaypointCells(map, players, corners, spawns);
            return spawns;
        }

        private static List<Point> PlaceReferenceWaypoints(
            Map map,
            int players,
            IReadOnlyList<int> referenceCells,
            List<Point> spawns)
        {
            const int globalWidth = 128;
            int count = Math.Min(players, referenceCells.Count);
            var tiles = new Point[count];
            for (int i = 0; i < count; i++)
            {
                int globalCell = referenceCells[i];
                tiles[i] = new Point(globalCell % globalWidth, globalCell / globalWidth);
            }

            AssignWaypointCells(map, players, tiles, spawns);
            return spawns;
        }

        private static void AssignWaypointCells(Map map, int players, IReadOnlyList<Point> tiles, List<Point> spawns)
        {
            for (int i = 0; i < players && i < map.Waypoints.Length && i < tiles.Count; i++)
            {
                var wp = map.Waypoints[i];
                if (wp.Flag != WaypointFlag.PlayerStart)
                {
                    continue;
                }
                var tile = tiles[i];
                if (!map.Metrics.GetCell(tile, out int cell))
                {
                    continue;
                }
                wp.Cell = cell;
                spawns.Add(tile);
            }
        }
    }
}